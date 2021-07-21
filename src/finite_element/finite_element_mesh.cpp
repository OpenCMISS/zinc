/**
 * FILE : finite_element_mesh.cpp
 *
 * Class defining a domain consisting of a set of finite elements.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/element.h"
#include "computed_field/field_derivative.hpp"
#include "computed_field/fieldparametersprivate.hpp"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_region_private.h"
#include "general/object.h"
#include "general/debug.h"
#include "general/message.h"
#include "general/mystring.h"
#include <algorithm>

/*
Module types
------------
*/

FE_element_template::FE_field_data::FE_field_data(FE_field *fieldIn) :
	field(ACCESS(FE_field)(fieldIn)),
	componentCount(get_FE_field_number_of_components(fieldIn)),
	efts(0),
	cacheComponentMeshFieldTemplates(new FE_mesh_field_template*[this->componentCount])
{
	for (int i = 0; i < this->componentCount; ++i)
		cacheComponentMeshFieldTemplates[i] = 0;
}

FE_element_template::FE_field_data::~FE_field_data()
{
	this->undefine();
	delete[] this->cacheComponentMeshFieldTemplates;
	DEACCESS(FE_field)(&this->field);
}

int FE_element_template::FE_field_data::define(int componentNumber, FE_element_field_template *eft)
{
	if ((componentNumber >= this->componentCount) || (!eft))
		return CMZN_ERROR_ARGUMENT;
	if (!this->efts)
	{
		this->efts = new FE_element_field_template*[this->componentCount];
		if (!this->efts)
			return CMZN_ERROR_MEMORY;
		for (int i = 0; i < this->componentCount; ++i)
			this->efts[i] = 0;
	}
	if (componentNumber >= 0)
	{
		if (this->efts[componentNumber])
			FE_element_field_template::deaccess(this->efts[componentNumber]);
		this->efts[componentNumber] = eft->access();
	}
	else
	{
		for (int i = 0; i < this->componentCount; ++i)
		{
			if (this->efts[i])
				FE_element_field_template::deaccess(this->efts[i]);
			this->efts[i] = eft->access();
		}
	}
	return CMZN_OK;
}

void FE_element_template::FE_field_data::undefine()
{
	if (this->efts)
	{
		for (int i = 0; i < this->componentCount; ++i)
			FE_element_field_template::deaccess(this->efts[i]);
		delete[] this->efts;
		this->efts = 0;
	}
}

bool FE_element_template::FE_field_data::validate() const
{
	if (this->efts)
	{
		for (int i = 0; i < this->componentCount; ++i)
			if (!this->efts[i])
				return false;
	}
	return true;
}

bool FE_element_template::FE_field_data::mergeElementfieldtemplatesIntoMesh(FE_mesh &mesh)
{
	if (this->efts)
	{
		for (int i = 0; i < this->componentCount; ++i)
		{
			if (this->efts[i])
			{
				FE_element_field_template *mergedEFT = mesh.mergeElementfieldtemplate(this->efts[i]);
				if (!mergedEFT)
					return false;
				FE_element_field_template *tmpEFT = this->efts[i];
				this->efts[i] = mergedEFT;
				FE_element_field_template::deaccess(tmpEFT);
			}
			else
				return false;
		}
	}
	return true;
}

FE_element_template::FE_element_template(FE_mesh *mesh_in, FE_element_shape *elementShapeIn) :
	cmzn::RefCounted(),
	mesh(mesh_in->access()),
	elementShape(elementShapeIn ? ACCESS(FE_element_shape)(elementShapeIn) : 0),
	fieldCount(0),
	fields(0),
	fieldsValidated(true)
{
}

FE_element_template::~FE_element_template()
{
	this->mesh->noteChangedElementTemplate(this);
	for (int i = 0; i < fieldCount; ++i)
		delete this->fields[i];
	delete[] this->fields;
	FE_mesh::deaccess(this->mesh);
	if (this->elementShape)
		DEACCESS(FE_element_shape)(&(this->elementShape));
}

/** @param field  The field to add, must have already been checked */
int FE_element_template::addField(FE_field *field)
{
	FE_field_data *fieldData = new FE_field_data(field);
	if (!fieldData)
		return -1;
	FE_field_data **newFields = new FE_field_data*[this->fieldCount + 1];
	if (!newFields)
	{
		delete fieldData;
		return -1;
	}
	for (int i = 0; i < this->fieldCount; ++i)
		newFields[i] = this->fields[i];
	newFields[this->fieldCount] = fieldData;
	delete[] this->fields;
	this->fields = newFields;
	++(this->fieldCount);
	return (this->fieldCount - 1);
}

int FE_element_template::getFieldIndex(FE_field *field) const
{
	for (int i = this->fieldCount - 1; 0 <= i; --i)
		if (this->fields[i]->getField() == field)
			return i;
	return -1;
}

int FE_element_template::setElementShape(FE_element_shape *elementShapeIn)
{
	if ((!elementShapeIn) || (get_FE_element_shape_dimension(elementShapeIn) == this->mesh->getDimension()))
	{
		REACCESS(FE_element_shape)(&this->elementShape, elementShapeIn);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int FE_element_template::defineField(FE_field *field, int componentNumber, FE_element_field_template *eft)
{
	if (!((field) && (componentNumber < get_FE_field_number_of_components(field)) && (eft)))
	{
		display_message(ERROR_MESSAGE, "Elementtemplate::defineField.  Invalid arguments");
		return CMZN_ERROR_ARGUMENT;
	}
	// GRC check FE_field is general real/integer?
	if (field->get_FE_region() != this->mesh->get_FE_region())
	{
		display_message(ERROR_MESSAGE, "Elementtemplate::defineField.  "
			"Field %s is not from this region", get_FE_field_name(field));
		return CMZN_ERROR_ARGUMENT;
	}
	if (eft->getMesh() != this->mesh)
	{
		display_message(ERROR_MESSAGE, "Elementtemplate::defineField.  "
			"Element field template is not for this mesh or region");
		return CMZN_ERROR_ARGUMENT;
	}
	if (!eft->isLocked())
	{
		display_message(ERROR_MESSAGE, "Elementtemplate::defineField.  "
			"Element field template is not locked");
		return CMZN_ERROR_ARGUMENT;
	}
	this->fieldsValidated = false;
	int fieldIndex = this->getFieldIndex(field);
	if (fieldIndex < 0)
	{
		fieldIndex = this->addField(field);
		if (fieldIndex < 0)
			return CMZN_ERROR_MEMORY;
		int result = this->fields[fieldIndex]->define(componentNumber, eft);
		if (result != CMZN_OK)
			this->removeField(field);
		return result;
	}
	this->mesh->noteChangedElementTemplate(this);
	return this->fields[fieldIndex]->define(componentNumber, eft);
}

int FE_element_template::defineField(FE_field *field, int componentNumber, cmzn_elementfieldtemplate *eftExt)
{
	if (!eftExt)
		return CMZN_ERROR_ARGUMENT;
	if (!eftExt->validateAndLock())
	{
		display_message(ERROR_MESSAGE, "Elementtemplate::defineField.  Element field template is invalid or incomplete");
		return CMZN_ERROR_ARGUMENT;
	}
	return this->defineField(field, componentNumber, eftExt->get_FE_element_field_template());
}

FE_element_field_template *FE_element_template::getElementfieldtemplate(FE_field *field, int componentNumber) const
{
	const int fieldIndex = this->getFieldIndex(field);
	if (fieldIndex < 0)
		return 0;
	return this->fields[fieldIndex]->getComponentElementfieldtemplate(componentNumber);
}

int FE_element_template::removeField(FE_field *field)
{
	const int fieldIndex = this->getFieldIndex(field);
	if (fieldIndex < 0)
		return CMZN_ERROR_NOT_FOUND;
	delete this->fields[fieldIndex];
	--(this->fieldCount);
	for (int i = fieldIndex; i < this->fieldCount; ++i)
		this->fields[i] = this->fields[i + 1];
	this->mesh->noteChangedElementTemplate(this);
	return CMZN_OK;
}

int FE_element_template::undefineField(FE_field *field)
{
	if (!field)
		return CMZN_ERROR_ARGUMENT;
	if (this->mesh->get_FE_region() != field->get_FE_region())
	{
		display_message(ERROR_MESSAGE, "Elementtemplate::undefineField.  "
			"Field %s is not from this region", get_FE_field_name(field));
		return CMZN_ERROR_ARGUMENT;
	}
	int fieldIndex = this->getFieldIndex(field);
	if (fieldIndex < 0)
	{
		fieldIndex = this->addField(field);
		if (fieldIndex < 0)
			return CMZN_ERROR_MEMORY;
	}
	else
	{
		this->fields[fieldIndex]->undefine();
	}
	this->mesh->noteChangedElementTemplate(this);
	return CMZN_OK;
}

bool FE_element_template::validate()
{
	if (!this->fieldsValidated)
	{
		this->fieldsValidated = true;
		for (int i = 0; i < this->fieldCount; ++i)
			if (!this->fields[i]->validate())
			{
				this->fieldsValidated = false;
				break;
			}
		if (this->fieldsValidated)
		{
			for (int i = 0; i < this->fieldCount; ++i)
				if (!this->fields[i]->mergeElementfieldtemplatesIntoMesh(*this->mesh))
				{
					this->fieldsValidated = false;
					break;
				}
		}
	}
	return this->fieldsValidated;
}

cmzn_element::~cmzn_element()
{
	if ((this->mesh) || (this->index != DS_LABEL_IDENTIFIER_INVALID))
	{
		display_message(ERROR_MESSAGE, "~cmzn_element.  Element has not been invalidated. Dimension %d Index %d",
			this->mesh ? this->mesh->getDimension() : 0, this->index);
	}
	if (0 != this->access_count)
	{
		display_message(ERROR_MESSAGE, "~cmzn_element.  Element destroyed with non-zero access count %d. Dimension %d Index %d",
			this->access_count, this->mesh ? this->mesh->getDimension() : -1, this->index);
	}
}

cmzn_element *cmzn_element::getAncestorConversion(FE_mesh *ancestorMesh, FE_value *elementToAncestor)
{
	if (!((this) && (this->mesh) && (elementToAncestor)))
	{
		display_message(ERROR_MESSAGE, "cmzn_element::getAncestorConversion.  Invalid argument(s)");
		return nullptr;
	}
	if (this->mesh == ancestorMesh)
	{
		elementToAncestor = nullptr;
		return this;
	}
	FE_mesh *parentMesh = this->mesh->getParentMesh();
	const DsLabelIndex *parents;
	const int parentsCount = this->mesh->getElementParents(this->index, parents);
	if ((!parentMesh) || (0 == parentsCount))
		return nullptr;
	for (int p = 0; p < parentsCount; ++p)
	{
		DsLabelIndex parentIndex = parents[p];
		if (parentIndex < 0)
			continue;
		cmzn_element *parent = parentMesh->getElement(parentIndex);
		FE_element_shape *parentShape = parentMesh->getElementShape(parentIndex);
		const int faceNumber = parentMesh->getElementFaceNumber(parentIndex, this->index);
		if ((parentShape) && (faceNumber >= 0))
		{
			cmzn_element *ancestorElement = (parentMesh == ancestorMesh) ? parent :
				parent->getAncestorConversion(ancestorMesh, elementToAncestor);
			if (!ancestorElement)
				continue;
			const FE_value *faceToElement = get_FE_element_shape_face_to_element(parentShape, faceNumber);
			if (!faceToElement)
				continue;
			const int size = ancestorElement->getDimension();
			if (parent == ancestorElement)
			{
				for (int i = size*size - 1; 0 <= i; --i)
					elementToAncestor[i] = faceToElement[i];
			}
			else
			{
				// multiply faceToElement of ancestorElement (in elementToAncestor)
				// by faceToElement from parent. This is the 1:3 case
				for (int i = 0; i < size; ++i)
				{
					elementToAncestor[i*2] = elementToAncestor[i*size] +
						elementToAncestor[i*size+1]*faceToElement[0]+
						elementToAncestor[i*size+2]*faceToElement[2];
					elementToAncestor[i*2+1] =
						elementToAncestor[i*size+1]*faceToElement[1]+
						elementToAncestor[i*size+2]*faceToElement[3];
				}
			}
			return ancestorElement;
		}
	}
	display_message(ERROR_MESSAGE, "cmzn_element::getAncestorConversion.  No ancestor found");
	return nullptr;
}

/** Free dynamic memory or resources held per-element e.g. reduce node element usage counts
  * @param elementIndex  Not checked, must be >= 0 */
void FE_mesh_element_field_template_data::clearElementVaryingData(DsLabelIndex elementIndex)
{
	// must release scale factors first as may be node-indexed
	if (this->localScaleFactorCount > 0)
	{
		this->destroyElementScaleFactorIndexes(elementIndex);
	}
	FE_mesh *mesh = this->eft->getMesh();
	if (mesh)
	{
		if (this->localNodeCount > 0)
		{
			FE_nodeset *nodeset = mesh->getNodeset();
			if (nodeset)
			{
				DsLabelIndex *nodeIndexes = this->getElementNodeIndexes(elementIndex);
				if (nodeIndexes)
				{
					for (int i = 0; i < this->localNodeCount; ++i)
						nodeset->decrementElementUsageCount(nodeIndexes[i]);
				}
			}
		}
	}
	this->localToGlobalNodes.destroyArray(elementIndex);
}

/** Free dynamic memory or resources held per-element e.g. reduce node element usage counts */
void FE_mesh_element_field_template_data::clearAllElementVaryingData()
{
	FE_mesh *mesh = this->eft->getMesh();
	if (mesh)
	{
		// must release scale factors first as may be node-indexed
		if (this->localScaleFactorCount > 0)
		{
			const DsLabelIndex elementIndexLimit = this->getElementIndexLimit();
			for (DsLabelIndex elementIndex = this->getElementIndexStart(); elementIndex < elementIndexLimit; ++elementIndex)
			{
				this->destroyElementScaleFactorIndexes(elementIndex);
			}
		}
		if (this->localNodeCount > 0)
		{
			FE_nodeset *nodeset = mesh->getNodeset();
			if (nodeset)
			{
				const DsLabelIndex elementIndexLimit = this->getElementIndexLimit();
				for (DsLabelIndex elementIndex = this->getElementIndexStart(); elementIndex < elementIndexLimit; ++elementIndex)
				{
					DsLabelIndex *nodeIndexes = this->getElementNodeIndexes(elementIndex);
					if (nodeIndexes)
					{
						for (int i = 0; i < this->localNodeCount; ++i)
							nodeset->decrementElementUsageCount(nodeIndexes[i]);
					}
				}
			}
		}
	}
	if (this->localScaleFactorCount > 0)
	{
		this->localToGlobalScaleFactors.clear();
	}
	this->localToGlobalNodes.clear();
	this->meshfieldtemplateUsageCount.clear();
}

/** Call when start using this eft data in a field template for element at elementIndex.
  * @param elementIndex  Not checked, assume valid. */
void FE_mesh_element_field_template_data::incrementMeshfieldtemplateUsageCount(DsLabelIndex elementIndex)
{
	MeshfieldtemplateUsageCountType *meshfieldtemplateUsageCountAddress = this->meshfieldtemplateUsageCount.getAddress(elementIndex);
	if (meshfieldtemplateUsageCountAddress)
		++(*meshfieldtemplateUsageCountAddress);
	else
		this->meshfieldtemplateUsageCount.setValue(elementIndex, 1);
}

/** Call when stop using this eft data in a field template for element at elementIndex.
  * Frees element-varying data when usage count drops to zero.
  * @param elementIndex  Not checked, assume valid. */
void FE_mesh_element_field_template_data::decrementMeshfieldtemplateUsageCount(DsLabelIndex elementIndex)
{
	MeshfieldtemplateUsageCountType *meshfieldtemplateUsageCountAddress = this->meshfieldtemplateUsageCount.getAddress(elementIndex);
	if (meshfieldtemplateUsageCountAddress)
	{
		if (*meshfieldtemplateUsageCountAddress > 0)
		{
			--(*meshfieldtemplateUsageCountAddress);
			if (0 == *meshfieldtemplateUsageCountAddress)
				this->clearElementVaryingData(elementIndex);
		}
		else
			display_message(ERROR_MESSAGE, "FE_mesh_element_field_template_data::decrementMeshfieldtemplateUsageCount.  Count is already 0");
	}
	else
		display_message(ERROR_MESSAGE, "FE_mesh_element_field_template_data::decrementMeshfieldtemplateUsageCount.  Missing count");
}

DsLabelIndex FE_mesh_element_field_template_data::getElementFirstNodeIndex(DsLabelIndex elementIndex) const
{
	if (this->eft->getParameterMappingMode() != CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
	{
		return DS_LABEL_INDEX_INVALID;
	}
	const int localNodeIndex = this->eft->getTermLocalNodeIndex(0, 0);
	if (localNodeIndex < 0)
	{
		return DS_LABEL_INDEX_INVALID;
	}
	const DsLabelIndex *nodeIndexes = this->getElementNodeIndexes(elementIndex);
	if (!nodeIndexes)
	{
		return DS_LABEL_INDEX_INVALID;
	}
	return nodeIndexes[localNodeIndex];
}

DsLabelIndex FE_mesh_element_field_template_data::getElementLastNodeIndex(DsLabelIndex elementIndex) const
{
	if (this->eft->getParameterMappingMode() != CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
	{
		return DS_LABEL_INDEX_INVALID;
	}
	const int localNodeIndex = this->eft->getTermLocalNodeIndex(this->eft->getNumberOfFunctions() - 1, 0);
	if (localNodeIndex < 0)
	{
		return DS_LABEL_INDEX_INVALID;
	}
	const DsLabelIndex *nodeIndexes = this->getElementNodeIndexes(elementIndex);
	if (!nodeIndexes)
	{
		return DS_LABEL_INDEX_INVALID;
	}
	return nodeIndexes[localNodeIndex];
}

/** @return  Upper limit of element indexes for which EFT is set.
  * Value is at least one greater than last index using EFT. */
DsLabelIndex FE_mesh_element_field_template_data::getElementIndexLimit() const
{
	DsLabelIndex elementIndexLimit = this->meshfieldtemplateUsageCount.getIndexLimit();
	const FE_mesh *mesh = this->eft->getMesh();
	if (mesh && (mesh->getLabelsIndexSize() < elementIndexLimit))
		elementIndexLimit = mesh->getLabelsIndexSize();
	return elementIndexLimit;
}

/** @return  Lowest element index for which EFT is probably set */
DsLabelIndex FE_mesh_element_field_template_data::getElementIndexStart() const
{
	return this->meshfieldtemplateUsageCount.getIndexStart();
}

int FE_mesh_element_field_template_data::setElementLocalNode(DsLabelIndex elementIndex, int localNodeIndex, DsLabelIndex nodeIndex)
{
	if (elementIndex < 0)
		return CMZN_ERROR_ARGUMENT;
	FE_mesh *mesh = this->eft->getMesh();
	if (!mesh)
		return CMZN_ERROR_ARGUMENT;
	FE_nodeset *nodeset = mesh->getNodeset();
	if (!nodeset)
		return CMZN_ERROR_ARGUMENT;
	DsLabelIndex *elementNodeIndexes = this->getOrCreateElementNodeIndexes(elementIndex);
	if (!elementNodeIndexes)
	{
		display_message(ERROR_MESSAGE,
			"FE_mesh_element_field_template_data::setElementLocalNode.  Failed to create element node indexes");
		return CMZN_ERROR_MEMORY;
	}
	if ((nodeIndex < 0) && (elementNodeIndexes[localNodeIndex] >= 0))
	{
		display_message(ERROR_MESSAGE, "Element setNode.  Cannot clear a node at a local index");
		return CMZN_ERROR_ARGUMENT;
	}
	if (nodeIndex != elementNodeIndexes[localNodeIndex])
	{
		if ((elementNodeIndexes[localNodeIndex] >= 0) && (this->eft->hasNodeScaleFactors()))
		{
			std::vector<DsLabelIndex> newNodeIndexes(this->localNodeCount);
			for (int n = 0; n < this->localNodeCount; ++n)
			{
				newNodeIndexes[n] = elementNodeIndexes[n];
			}
			newNodeIndexes[localNodeIndex] = nodeIndex;
			if (!this->updateElementNodeScaleFactorIndexes(elementIndex, newNodeIndexes.data()))
			{
				display_message(ERROR_MESSAGE,
					"FE_mesh_element_field_template_data::setElementLocalNode.  Failed to update node-based scale factors. No changes made.");
				return CMZN_ERROR_MEMORY;
			}
		}
		if (nodeIndex >= 0)
			nodeset->incrementElementUsageCount(nodeIndex);
		if (elementNodeIndexes[localNodeIndex] >= 0)
			nodeset->decrementElementUsageCount(elementNodeIndexes[localNodeIndex]);
		elementNodeIndexes[localNodeIndex] = nodeIndex;
		// simplest to mark all fields as changed as they may share local nodes
		// can optimise in future
		// following will update clients:
		mesh->elementAllFieldChange(elementIndex, DS_LABEL_CHANGE_TYPE_DEFINITION);
	}
	return CMZN_OK;
}

int FE_mesh_element_field_template_data::setElementLocalNodes(
	DsLabelIndex elementIndex, const DsLabelIndex *nodeIndexes)
{
	if (elementIndex < 0)
		return CMZN_ERROR_ARGUMENT;
	FE_mesh *mesh = this->eft->getMesh();
	if (!mesh)
		return CMZN_ERROR_ARGUMENT;
	FE_nodeset *nodeset = mesh->getNodeset();
	if (!nodeset)
		return CMZN_ERROR_ARGUMENT;
	DsLabelIndex *elementNodeIndexes = this->getOrCreateElementNodeIndexes(elementIndex);
	if (!elementNodeIndexes)
	{
		display_message(ERROR_MESSAGE,
			"FE_mesh_element_field_template_data::setElementLocalNodes.  Failed to create element node indexes");
		return CMZN_ERROR_MEMORY;
	}
	// set following to true if an existing valid node indexes is changed
	bool localNodeChangeFromValid = false;
	for (int n = 0; n < this->localNodeCount; ++n)
	{
		if ((elementNodeIndexes[n] >= 0) && (nodeIndexes[n] != elementNodeIndexes[n]))
		{
			localNodeChangeFromValid = true;
			if (nodeIndexes[n] < 0)
			{
				display_message(ERROR_MESSAGE, "Element setNodes.  Cannot clear a node at a local index");
				return CMZN_ERROR_ARGUMENT;
			}
		}
	}
	if (localNodeChangeFromValid && (this->eft->hasNodeScaleFactors()))
	{
		if (!this->updateElementNodeScaleFactorIndexes(elementIndex, nodeIndexes))
		{
			display_message(ERROR_MESSAGE,
				"FE_mesh_element_field_template_data::setElementLocalNodes.  Failed to update node-based scale factors. No changes made.");
			return CMZN_ERROR_MEMORY;
		}
	}
	bool change = false;
	for (int n = 0; n < this->localNodeCount; ++n)
	{
		if (nodeIndexes[n] != elementNodeIndexes[n])
		{
			change = true;
			if (nodeIndexes[n] >= 0)
			{
				nodeset->incrementElementUsageCount(nodeIndexes[n]);
			}
			if (elementNodeIndexes[n] >= 0)
			{
				nodeset->decrementElementUsageCount(elementNodeIndexes[n]);
			}
			elementNodeIndexes[n] = nodeIndexes[n];
		}
	}
	if (change)
	{
		// simplest to mark all fields as changed since too expensive to determine which are affected
		// can optimise in future
		// following will update clients:
		mesh->elementAllFieldChange(elementIndex, DS_LABEL_CHANGE_TYPE_DEFINITION);
	}
	return CMZN_OK;
}

int FE_mesh_element_field_template_data::setElementLocalNodesByIdentifier(
	DsLabelIndex elementIndex, const DsLabelIdentifier *nodeIdentifiers)
{
	if ((elementIndex < 0) || (this->localNodeCount == 0) || (nodeIdentifiers == 0))
		return CMZN_ERROR_ARGUMENT;
	FE_mesh *mesh = this->eft->getMesh();
	if (!mesh)
		return CMZN_ERROR_ARGUMENT;
	FE_nodeset *nodeset = mesh->getNodeset();
	if (!nodeset)
		return CMZN_ERROR_ARGUMENT;
	std::vector<DsLabelIndex> nodeIndexes(this->localNodeCount, DS_LABEL_INDEX_INVALID);
	for (int n = 0; n < this->localNodeCount; ++n)
	{
		if (nodeIdentifiers[n] >= 0)
		{
			if ((nodeIndexes[n] = nodeset->findIndexByIdentifier(nodeIdentifiers[n])) == DS_LABEL_INDEX_INVALID)
			{
				display_message(ERROR_MESSAGE,
					"Element setNodesByIdentifier.  Failed to find node %d to set as local node %d/%d in %d-D element %d",
					nodeIdentifiers[n], n + 1, this->localNodeCount, mesh->getDimension(), mesh->getElementIdentifier(elementIndex));
				return CMZN_ERROR_ARGUMENT;
			}
		}
	}
	return this->setElementLocalNodes(elementIndex, nodeIndexes.data());
}

int FE_mesh_element_field_template_data::getElementScaleFactor(DsLabelIndex elementIndex, int localScaleFactorIndex, FE_value& value)
{
	if (elementIndex < 0)
		return CMZN_ERROR_ARGUMENT;
	FE_mesh *mesh = this->eft->getMesh();
	if (!mesh)
		return CMZN_ERROR_ARGUMENT;
	int result = CMZN_OK;
	const DsLabelIndex *scaleFactorIndexes = this->getOrCreateElementScaleFactorIndexes(result, elementIndex);
	if (!scaleFactorIndexes)
	{
		display_message(ERROR_MESSAGE, "Element getScaleFactor.  Element has no scale factors");
		return result;
	}
	value = mesh->getScaleFactor(scaleFactorIndexes[localScaleFactorIndex]);
	return CMZN_OK;
}

int FE_mesh_element_field_template_data::setElementScaleFactor(DsLabelIndex elementIndex, int localScaleFactorIndex, FE_value value)
{
	if (elementIndex < 0)
		return CMZN_ERROR_ARGUMENT;
	FE_mesh *mesh = this->eft->getMesh();
	if (!mesh)
		return CMZN_ERROR_ARGUMENT;
	int result = CMZN_OK;
	DsLabelIndex *scaleFactorIndexes = this->getOrCreateElementScaleFactorIndexes(result, elementIndex);
	if (!scaleFactorIndexes)
	{
		display_message(ERROR_MESSAGE, "Element setScaleFactor.  Element has no scale factors");
		return result;
	}
	mesh->setScaleFactor(scaleFactorIndexes[localScaleFactorIndex], value);
	// conservatively mark all fields as changed as they may share scale factors
	// can optimise in future
	// following will update clients:
	mesh->elementAllFieldChange(elementIndex, DS_LABEL_CHANGE_TYPE_DEFINITION);
	return CMZN_OK;
}

int FE_mesh_element_field_template_data::getElementScaleFactors(DsLabelIndex elementIndex, FE_value *valuesOut) const
{
	if (elementIndex < 0)
		return CMZN_ERROR_ARGUMENT;
	FE_mesh *mesh = this->eft->getMesh();
	if (!mesh)
		return CMZN_ERROR_ARGUMENT;
	const DsLabelIndex *scaleFactorIndexes = this->getElementScaleFactorIndexes(elementIndex);
	if (!scaleFactorIndexes)
	{
		display_message(ERROR_MESSAGE, "Element getScaleFactors.  Element has no scale factors");
		return CMZN_ERROR_NOT_FOUND;
	}
	const int count = this->eft->getNumberOfLocalScaleFactors();
	for (int i = 0; i < count; ++i)
	{
		valuesOut[i] = mesh->getScaleFactor(scaleFactorIndexes[i]);
	}
	return CMZN_OK;
}

int FE_mesh_element_field_template_data::getOrCreateElementScaleFactors(DsLabelIndex elementIndex, FE_value *valuesOut)
{
	if (elementIndex < 0)
		return CMZN_ERROR_ARGUMENT;
	FE_mesh *mesh = this->eft->getMesh();
	if (!mesh)
		return CMZN_ERROR_ARGUMENT;
	int result = CMZN_OK;
	const DsLabelIndex *scaleFactorIndexes = this->getOrCreateElementScaleFactorIndexes(result, elementIndex);
	if (!scaleFactorIndexes)
	{
		display_message(ERROR_MESSAGE, "Element getOrCreateElementScaleFactors.  Element has no scale factors");
		return result;
	}
	const int count = this->eft->getNumberOfLocalScaleFactors();
	for (int i = 0; i < count; ++i)
	{
		valuesOut[i] = mesh->getScaleFactor(scaleFactorIndexes[i]);
	}
	return CMZN_OK;
}

int FE_mesh_element_field_template_data::setElementScaleFactors(DsLabelIndex elementIndex, const FE_value *valuesIn)
{
	if (elementIndex < 0)
		return CMZN_ERROR_ARGUMENT;
	FE_mesh *mesh = this->eft->getMesh();
	if (!mesh)
		return CMZN_ERROR_ARGUMENT;
	int result = CMZN_OK;
	DsLabelIndex *scaleFactorIndexes = this->getOrCreateElementScaleFactorIndexes(result, elementIndex);
	if (!scaleFactorIndexes)
	{
		display_message(ERROR_MESSAGE, "Element setScaleFactors.  Element has no scale factors");
		return result;
	}
	const int count = this->eft->getNumberOfLocalScaleFactors();
	for (int i = 0; i < count; ++i)
	{
		mesh->setScaleFactor(scaleFactorIndexes[i], valuesIn[i]);
	}
	// conservatively mark all fields as changed as they may share scale factors
	// can optimise in future
	// following will update clients:
	mesh->elementAllFieldChange(elementIndex, DS_LABEL_CHANGE_TYPE_DEFINITION);
	return CMZN_OK;
}

/** @return  True if local to global nodes stored for all elements of mesh */
bool FE_mesh_element_field_template_data::localToGlobalNodesIsDense() const
{
	const FE_mesh *mesh = this->eft->getMesh();
	const DsLabelIndex elementIndexLimit = mesh->getLabelsIndexSize();
	for (DsLabelIndex elementIndex = 0; elementIndex < elementIndexLimit; ++elementIndex)
	{
		if (mesh->getElementIdentifier(elementIndex) >= 0)
		{
			if (this->localToGlobalNodes.getArray(elementIndex) == 0)
				return false;
		}
	}
	return true;
}

/** @return  Number of elements with a local to global node map */
int FE_mesh_element_field_template_data::getElementLocalToGlobalNodeMapCount() const
{
	int count = 0;
	const FE_mesh *mesh = this->eft->getMesh();
	const DsLabelIndex elementIndexLimit = mesh->getLabelsIndexSize();
	for (DsLabelIndex elementIndex = 0; elementIndex < elementIndexLimit; ++elementIndex)
	{
		if (mesh->getElementIdentifier(elementIndex) >= 0)
		{
			if (this->localToGlobalNodes.getArray(elementIndex) != 0)
				++count;
		}
	}
	return count;
}

/** Merges local-to-global node map, scale factors from source, finding target
  * elements and nodes by identifier.
  * Used only by FE_mesh::merge to merge data from another region.
  * Must have already merged mesh field templates such that the target element field
  * template expects the right numbers of nodes, scale factors.
  * Warns about changes to element local nodes.
  * Should only call if source.hasElementVaryingData() returns true.
  * @param source  Source element field template data to merge.
  * @return  True on success, false on failure. */
bool FE_mesh_element_field_template_data::mergeElementVaryingData(const FE_mesh_element_field_template_data& source)
{
	const FE_mesh *sourceMesh = source.eft->getMesh();
	const FE_mesh *targetMesh = this->eft->getMesh();
	const FE_nodeset *sourceNodeset = sourceMesh->getNodeset();
	// following is non-const since changing nodes' element usage counts
	FE_nodeset *targetNodeset = targetMesh->getNodeset();
	// the above should all exist at merge time
	bool localNodeChange = false;
	DsLabelIdentifier localNodeChangeElementIdentifier = DS_LABEL_IDENTIFIER_INVALID;
	const DsLabelIndex sourceElementIndexLimit = source.getElementIndexLimit();
	for (DsLabelIndex sourceElementIndex = source.getElementIndexStart(); sourceElementIndex < sourceElementIndexLimit; ++sourceElementIndex)
	{
		const DsLabelIdentifier elementIdentifier = sourceMesh->getElementIdentifier(sourceElementIndex);
		if (DS_LABEL_IDENTIFIER_INVALID == elementIdentifier)
			continue; // no element at that index
		if (source.meshfieldtemplateUsageCount.getValue(sourceElementIndex) == 0)
			continue; // no data at that index
		const DsLabelIndex targetElementIndex = targetMesh->findIndexByIdentifier(elementIdentifier);
		// the above must be valid if merge has got to this point, so not testing.
		if (this->localNodeCount > 0)
		{
			DsLabelIndex *sourceNodes = source.localToGlobalNodes.getArray(sourceElementIndex);
			if (!sourceNodes)
			{
				display_message(ERROR_MESSAGE, "FE_mesh_element_field_template_data::mergeElementVaryingData.  "
					"Missing source local nodes for element %d", elementIdentifier);
				return false;
			}
			DsLabelIndex *targetNodes = this->localToGlobalNodes.getOrCreateArray(targetElementIndex);
			if (!targetNodes)
			{
				display_message(ERROR_MESSAGE, "FE_mesh_element_field_template_data::mergeElementVaryingData.  "
					"Could not allocate target local nodes");
				return false;
			}
			for (int n = 0; n < this->localNodeCount; ++n)
			{
				const DsLabelIndex sourceNodeIndex = sourceNodes[n];
				DsLabelIndex targetNodeIndex;
				if (sourceNodeIndex < 0)
				{
					targetNodeIndex = DS_LABEL_INDEX_INVALID;
				}
				else
				{
					const DsLabelIdentifier nodeIdentifier = sourceNodeset->getNodeIdentifier(sourceNodeIndex);
					targetNodeIndex = targetNodeset->findIndexByIdentifier(nodeIdentifier);
					if (targetNodeIndex < 0)
					{
						display_message(ERROR_MESSAGE, "FE_mesh_element_field_template_data::mergeElementVaryingData.  "
							"Missing target node %d in element %d", nodeIdentifier, elementIdentifier);
						return false;
					}
				}
				if (targetNodes[n] != targetNodeIndex)
				{
					if (targetNodeIndex >= 0)
						targetNodeset->incrementElementUsageCount(targetNodeIndex);
					if (targetNodes[n] >= 0)
					{
						targetNodeset->decrementElementUsageCount(targetNodes[n]);
						if (!localNodeChange)
						{
							localNodeChangeElementIdentifier = elementIdentifier;
							localNodeChange = true;
						}
					}
					targetNodes[n] = targetNodeIndex;
				}
			}
		}
		if (this->localScaleFactorCount > 0)
		{
			// transfer scale factors only if source has any
			const DsLabelIndex *sourceScaleFactorIndexes = source.localToGlobalScaleFactors.getArray(sourceElementIndex);
			if (!sourceScaleFactorIndexes)
			{
				display_message(WARNING_MESSAGE, "FE_mesh_element_field_template_data::mergeElementVaryingData.  "
					"Missing source local scale factors for element %d", elementIdentifier);
			}
			else
			{
				std::vector<FE_value> scaleFactorsVector(this->localScaleFactorCount);
				FE_value *scaleFactors = scaleFactorsVector.data();
				for (int s = 0; s < this->localScaleFactorCount; ++s)
				{
					scaleFactors[s] = sourceMesh->getScaleFactor(sourceScaleFactorIndexes[s]);
				}
				if (CMZN_OK != this->setElementScaleFactors(targetElementIndex, scaleFactors))
				{
					display_message(ERROR_MESSAGE, "FE_mesh_element_field_template_data::mergeElementVaryingData.  "
						"Failed to set scale factors for element %d", elementIdentifier);
					return false;
				}
			}
		}
	}
	if (localNodeChange)
	{
		display_message(WARNING_MESSAGE, "Mesh merge.  Change in local-to-global node map. First found in %d-D element %d",
			targetMesh->getDimension(), localNodeChangeElementIdentifier);
	}
	return true;
}

DsLabelIndex *FE_mesh_element_field_template_data::createElementScaleFactorIndexes(int &result, DsLabelIndex elementIndex)
{
	if ((this->localScaleFactorCount == 0) || (elementIndex < 0))
	{
		display_message(ERROR_MESSAGE, "FE_mesh_element_field_template_data::createElementScaleFactorIndexes.  Invalid argument(s)");
		result = CMZN_ERROR_ARGUMENT;
		return 0;
	}
	const DsLabelIndex *nodeIndexes = 0;
	if (this->eft->hasNodeScaleFactors())
	{
		nodeIndexes = this->getElementNodeIndexes(elementIndex);
		if (!nodeIndexes)
		{
			display_message(ERROR_MESSAGE, "FE_mesh_element_field_template_data::createElementScaleFactorIndexes.  "
				"Can't create node type scale factors before local nodes are set");
			result = CMZN_ERROR_NOT_FOUND;
			return 0;
		}
	}
	DsLabelIndex *scaleFactorIndexes = this->localToGlobalScaleFactors.getOrCreateArray(elementIndex);
	if (scaleFactorIndexes)
	{
		FE_mesh *mesh = this->eft->getMesh();
		for (int s = 0; s < this->localScaleFactorCount; ++s)
		{
			const cmzn_elementfieldtemplate_scale_factor_type scaleFactorType = this->eft->getScaleFactorType(s);
			DsLabelIndex objectIndex = elementIndex;
			if (nodeIndexes && isScaleFactorTypeNode(scaleFactorType))
			{
				// validate guarantees all node-type scale factors have a valid local node index
				const int scaleFactorLocalNodeIndex = this->eft->getScaleFactorLocalNodeIndex(s);
				objectIndex = nodeIndexes[scaleFactorLocalNodeIndex]; // will be INVALID if local node note yet set
			}
			scaleFactorIndexes[s] = mesh->getOrCreateScaleFactorIndex(scaleFactorType, objectIndex, this->eft->getScaleFactorIdentifier(s));
			if (scaleFactorIndexes[s] < 0)
			{
				result = (isScaleFactorTypeNode(scaleFactorType) && (objectIndex < 0)) ? CMZN_ERROR_NOT_FOUND : CMZN_ERROR_GENERAL;
				this->destroyElementScaleFactorIndexes(elementIndex);
				display_message(ERROR_MESSAGE, "FE_mesh_element_field_template_data::createElementScaleFactorIndexes.  "
					"Failed to create scale factor%s", (result == CMZN_ERROR_NOT_FOUND) ? ", because local node not set for node-based scale factor" : ".");
				return 0;
			}
		}
	}
	result = CMZN_OK;
	return scaleFactorIndexes;
}

void FE_mesh_element_field_template_data::destroyElementScaleFactorIndexes(DsLabelIndex elementIndex)
{
	const DsLabelIndex *nodeIndexes = this->eft->hasNodeScaleFactors() ? this->getElementNodeIndexes(elementIndex) : 0;
	DsLabelIndex *scaleFactorIndexes = this->localToGlobalScaleFactors.getArray(elementIndex);
	if (scaleFactorIndexes)
	{
		FE_mesh *mesh = this->eft->getMesh();
		for (int s = 0; s < this->localScaleFactorCount; ++s)
		{
			if (scaleFactorIndexes[s] < 0) // expected only if called from failed createElementScaleFactorIndexes
				break;
			const cmzn_elementfieldtemplate_scale_factor_type scaleFactorType = this->eft->getScaleFactorType(s);
			DsLabelIndex objectIndex = elementIndex;
			if (nodeIndexes && isScaleFactorTypeNode(scaleFactorType))
			{
				objectIndex = nodeIndexes[this->eft->getScaleFactorLocalNodeIndex(s)];
			}
			mesh->releaseScaleFactorIndex(scaleFactorIndexes[s], scaleFactorType, objectIndex, this->eft->getScaleFactorIdentifier(s));
		}
		this->localToGlobalScaleFactors.clearArray(elementIndex);
	}
}

bool FE_mesh_element_field_template_data::updateElementNodeScaleFactorIndexes(DsLabelIndex elementIndex,
	const DsLabelIndex *newNodeIndexes)
{
	DsLabelIndex *scaleFactorIndexes = this->localToGlobalScaleFactors.getArray(elementIndex);
	if (!scaleFactorIndexes)
		return true; // no scale factors yet, so no update needed
	const DsLabelIndex *nodeIndexes = this->getElementNodeIndexes(elementIndex);
	FE_mesh *mesh = this->eft->getMesh();
	std::vector<DsLabelIndex> newScaleFactorIndexes(this->localScaleFactorCount);
	for (int s = 0; s < this->localScaleFactorCount; ++s)
	{
		newScaleFactorIndexes[s] = scaleFactorIndexes[s]; // will only become different if node scale factor and node changed
		const cmzn_elementfieldtemplate_scale_factor_type scaleFactorType = this->eft->getScaleFactorType(s);
		if (isScaleFactorTypeNode(scaleFactorType))
		{
			const int n = this->eft->getScaleFactorLocalNodeIndex(s);
			if (newNodeIndexes[n] != nodeIndexes[n])
			{
				const DsLabelIndex newScaleFactorIndex = mesh->getOrCreateScaleFactorIndex(
					scaleFactorType, newNodeIndexes[n], this->eft->getScaleFactorIdentifier(s));
				if (newScaleFactorIndex < 0)
				{
					// release only changed scale factor indexes with new nodes
					for (int s2 = 0; s2 < s; ++s2)
					{
						if (newScaleFactorIndexes[s2] != scaleFactorIndexes[s2])
						{
							const int n2 = this->eft->getScaleFactorLocalNodeIndex(s2);
							mesh->releaseScaleFactorIndex(newScaleFactorIndexes[s2], this->eft->getScaleFactorType(s2),
								newNodeIndexes[n2], this->eft->getScaleFactorIdentifier(s2));
						}
					}
					display_message(ERROR_MESSAGE, "FE_mesh_element_field_template_data::updateElementNodeScaleFactorIndexes.  Failed to create scale factor.");
					return false;
				}
				if (1 == mesh->getScaleFactorUsageCount(newScaleFactorIndex))
				{
					// newly created scale factor: copy previous value for element
					mesh->setScaleFactor(newScaleFactorIndex, mesh->getScaleFactor(scaleFactorIndexes[s]));
				}
				newScaleFactorIndexes[s] = newScaleFactorIndex;
			}
		}
	}
	// copy any changed indexes, but release old indexes with old nodes which are being changed
	for (int s = 0; s < this->localScaleFactorCount; ++s)
	{
		if (newScaleFactorIndexes[s] != scaleFactorIndexes[s])
		{
			const int n = this->eft->getScaleFactorLocalNodeIndex(s);
			mesh->releaseScaleFactorIndex(scaleFactorIndexes[s], this->eft->getScaleFactorType(s),
				nodeIndexes[n], this->eft->getScaleFactorIdentifier(s));
			scaleFactorIndexes[s] = newScaleFactorIndexes[s];
		}
	}
	return true;
}


FE_mesh_field_template::FE_mesh_field_template(const FE_mesh_field_template &source) :
	mesh(source.mesh),
	eftDataMap(source.eftDataMap),
	mapSize(source.mapSize),
	access_count(1)
{
	if (this->mapSize > 0)
	{
		// increment EFT element usage counts
		// iterate over mesh elements as efficiently as possible: in index order
		const DsLabelIndex elementIndexLimit = this->getElementIndexLimit();
		for (DsLabelIndex elementIndex = this->getElementIndexStart(); elementIndex < elementIndexLimit; ++elementIndex)
		{
			const DsLabelIdentifier elementIdentifier = this->mesh->getElementIdentifier(elementIndex);
			if (DS_LABEL_IDENTIFIER_INVALID == elementIdentifier)
				continue; // no element at that index
			const EFTIndexType eftIndex = this->eftDataMap.getValue(elementIndex);
			if (eftIndex != ELEMENT_FIELD_TEMPLATE_DATA_INDEX_INVALID)
			{
				FE_mesh_element_field_template_data *eftData = this->mesh->getElementfieldtemplateData(eftIndex);
				eftData->incrementMeshfieldtemplateUsageCount(elementIndex);
			}
		}
	}
}

FE_mesh_field_template::~FE_mesh_field_template()
{
	if (this->mesh)
	{
		if (this->mapSize > 0)
		{
			// decrement EFT element usage counts
			// iterate over mesh elements as efficiently as possible: in index order
			const DsLabelIndex elementIndexLimit = this->getElementIndexLimit();
			for (DsLabelIndex elementIndex = this->getElementIndexStart(); elementIndex < elementIndexLimit; ++elementIndex)
			{
				const DsLabelIdentifier elementIdentifier = this->mesh->getElementIdentifier(elementIndex);
				if (DS_LABEL_IDENTIFIER_INVALID == elementIdentifier)
					continue; // no element at that index
				const EFTIndexType eftIndex = this->eftDataMap.getValue(elementIndex);
				if (eftIndex != ELEMENT_FIELD_TEMPLATE_DATA_INDEX_INVALID)
				{
					FE_mesh_element_field_template_data *eftData = this->mesh->getElementfieldtemplateData(eftIndex);
					eftData->decrementMeshfieldtemplateUsageCount(elementIndex);
				}
			}
		}
		this->mesh->removeMeshFieldTemplate(this);
	}
}

/** @return  Upper limit of element indexes for which EFT is set. Returned value
  * is at least one greater than last index using EFT. */
DsLabelIndex FE_mesh_field_template::getElementIndexLimit() const
{
	DsLabelIndex elementIndexLimit = this->mesh->getLabelsIndexSize();
	DsLabelIndex mapLimit = this->eftDataMap.getIndexLimit();
	if (mapLimit < elementIndexLimit)
		return mapLimit;
	return elementIndexLimit;
}

/** @return  Lowest element index for which EFT index is allocated. May not be in-use though */
DsLabelIndex FE_mesh_field_template::getElementIndexStart() const
{
	return this->eftDataMap.getIndexStart();
}

/**
 * Sets element function by EFT Data index in mesh. Handles MFT usage counts.
 * @param elementIndex  Not checked; assume valid
 * @param eftIndex  Index of EFT Data in mesh or ELEMENT_FIELD_TEMPLATE_DATA_INDEX_INVALID to clear.
 * @return  True on success, false on failure.
 */
bool FE_mesh_field_template::setElementfieldtemplateIndex(DsLabelIndex elementIndex, EFTIndexType eftIndex)
{
	if (eftIndex >= 0)
	{
		FE_mesh_element_field_template_data *newEFTData = mesh->getElementfieldtemplateData(eftIndex);
		newEFTData->incrementMeshfieldtemplateUsageCount(elementIndex);
		++this->mapSize;
	}
	EFTIndexType *indexAddress = this->eftDataMap.getAddress(elementIndex);
	if (indexAddress)
	{
		if (*indexAddress != ELEMENT_FIELD_TEMPLATE_DATA_INDEX_INVALID)
		{
			FE_mesh_element_field_template_data *oldEFTData = this->mesh->getElementfieldtemplateData(*indexAddress);
			oldEFTData->decrementMeshfieldtemplateUsageCount(elementIndex);
			--this->mapSize;
		}
		*indexAddress = eftIndex;
	}
	else if (eftIndex >= 0) // don't allocate if clearing
		return this->eftDataMap.setValue(elementIndex, eftIndex);
	return true;
}

/**
 * Query whether this MFT is identical to source with the eft indexes remapped.
 * Used for merging MFTs from another region, hence mesh is not compared.
 * @see FE_mesh::merge()
 * @param source  Source mesh field template to compare with.
 * @param sourceEFTIndexMap  Vector mapping eft index in source MFT's mesh to
 * eft indexes for this MFT's mesh.
 * @param superset  If true, this MFT matches if superset of source, otherwise exact match.
 * @return  True if match, false if not.
 */
bool FE_mesh_field_template::matchesWithEFTIndexMap(const FE_mesh_field_template &source,
	const std::vector<EFTIndexType> &sourceEFTIndexMap, bool superset) const
{
	// try to trivially reject first, since expensive to match
	if (this->mapSize != source.mapSize)
		return false;
	if ((0 == this->mesh) || (0 == source.mesh))
		return false;
	// iterate over source mesh elements as efficiently as possible, in index order
	const DsLabelIndex sourceElementIndexLimit = source.getElementIndexLimit();
	for (DsLabelIndex sourceElementIndex = source.getElementIndexStart(); sourceElementIndex < sourceElementIndexLimit; ++sourceElementIndex)
	{
		const DsLabelIdentifier elementIdentifier = source.mesh->getElementIdentifier(sourceElementIndex);
		if (DS_LABEL_IDENTIFIER_INVALID == elementIdentifier)
			continue; // no element at that index. Shouldn't happen when merging newly-read input file
		EFTIndexType sourceEFTIndex;
		if ((!source.eftDataMap.getValue(sourceElementIndex, sourceEFTIndex))
			|| (sourceEFTIndex == ELEMENT_FIELD_TEMPLATE_DATA_INDEX_INVALID))
			continue; // field template not defined at that index
		const DsLabelIndex targetElementIndex = this->mesh->findIndexByIdentifier(elementIdentifier);
		if (DS_LABEL_INDEX_INVALID == targetElementIndex)
		{
			display_message(ERROR_MESSAGE, "FE_mesh_field_template::matchesWithEFTIndexMap.  Missing element.");
			return false; // unexpected; element should exist at this stage of merge
		}
		if (!this->eftDataMap.hasValue(targetElementIndex, sourceEFTIndexMap[sourceEFTIndex]))
			return false; // field template not defined or different value at that index => not a match
	}
	return true;
}

/**
 * Merge the source MFT into this MFT, translating source to target EFT indexes.
 * Used for merging MFTs from another region.
 * @see FE_mesh::merge()
 * @param source  Source mesh field template to merge in.
 * @param sourceEFTIndexMap  Vector mapping eft index in source MFT's mesh to
 * eft indexes for this MFT's mesh.
 * @return  True on success, false on failure.
 */
bool FE_mesh_field_template::mergeWithEFTIndexMap(const FE_mesh_field_template &source,
	const std::vector<EFTIndexType> &sourceEFTIndexMap)
{
	if ((0 == this->mesh) || (0 == source.mesh))
		return false;
	// iterate over source mesh elements as efficiently as possible, in index order
	const DsLabelIndex sourceElementIndexLimit = source.getElementIndexLimit();
	for (DsLabelIndex sourceElementIndex = source.getElementIndexStart(); sourceElementIndex < sourceElementIndexLimit; ++sourceElementIndex)
	{
		const DsLabelIdentifier elementIdentifier = source.mesh->getElementIdentifier(sourceElementIndex);
		if (DS_LABEL_IDENTIFIER_INVALID == elementIdentifier)
			continue; // no element at that index. Shouldn't happen when merging newly-read input file
		EFTIndexType sourceEFTIndex;
		if ((!source.eftDataMap.getValue(sourceElementIndex, sourceEFTIndex))
			|| (sourceEFTIndex == ELEMENT_FIELD_TEMPLATE_DATA_INDEX_INVALID))
			continue; // field template not defined at that index
		const DsLabelIndex targetElementIndex = this->mesh->findIndexByIdentifier(elementIdentifier);
		if (DS_LABEL_INDEX_INVALID == targetElementIndex)
		{
			display_message(ERROR_MESSAGE, "FE_mesh_field_template::mergeWithEFTIndexMap.  Missing element.");
			return false; // unexpected; element should exist at this stage of merge
		}
		if (!this->setElementfieldtemplateIndex(targetElementIndex, sourceEFTIndexMap[sourceEFTIndex]))
			return false; // failed to set
	}
	return true;
}

/** @return  True if any element of any component uses a non-linear basis in
  * any direction, chiefly used to control application of tessellation
  * refinement.
  * GRC Note:
  * This can be expensive; it could be made more efficient if the template
  * stored the count of elements using each EFT. However, rather than doing
  * this it would be better to make graphics query non-linearity per-element
  * and per xi direction and use tessellation refinements independently for
  * each. */
bool FE_mesh_field_template::usesNonLinearBasis() const
{
	std::vector<EFTIndexType> nonLinearEFTIndexes;
	const int eftIndexCount = this->getMesh()->getElementfieldtemplateDataCount();
	for (int i = 0; i < eftIndexCount; ++i)
	{
		FE_mesh_element_field_template_data *meshEFTData = this->getMesh()->getElementfieldtemplateData(i);
		if (meshEFTData && FE_basis_is_non_linear(meshEFTData->getElementfieldtemplate()->getBasis()))
			nonLinearEFTIndexes.push_back(static_cast<EFTIndexType>(i));
	}
	const int eftIndexesSize = static_cast<int>(nonLinearEFTIndexes.size());
	if (eftIndexesSize == 0)
		return false;
	const EFTIndexType *eftIndexes = nonLinearEFTIndexes.data();
	const DsLabelIndex elementIndexStart = this->getElementIndexStart();
	const DsLabelIndex elementIndexLimit = this->getElementIndexLimit();
	for (DsLabelIndex elementIndex = elementIndexStart; elementIndex < elementIndexLimit; ++elementIndex)
	{
		const EFTIndexType eftIndex = this->eftDataMap.getValue(elementIndex);
		if (eftIndex >= 0)
		{
			for (int i = 0; i < eftIndexesSize; ++i)
				if (eftIndexes[i] == eftIndex)
					return true;
		}
	}
	return false;
}

/** @return  Number of elements mft is defined on */
int FE_mesh_field_template::getElementsDefinedCount() const
{
	int count = 0;
	const FE_mesh *mesh = this->getMesh();
	const DsLabelIndex elementIndexLimit = this->getElementIndexLimit();
	for (DsLabelIndex elementIndex = this->getElementIndexStart(); elementIndex < elementIndexLimit; ++elementIndex)
	{
		if ((mesh->getElementIdentifier(elementIndex) >= 0) 
			&& (this->eftDataMap.getValue(elementIndex) >= 0))
			++count;
	}
	return count;
}

void FE_mesh_embedded_node_field::addNode(DsLabelIndex elementIndex, DsLabelIndex nodeIndex)
{
	DsLabelIndex **nodeIndexesAddress = this->map.getOrCreateAddress(elementIndex);
	DsLabelIndex *nodeIndexes = *nodeIndexesAddress;
	if (nodeIndexes)
	{
		const DsLabelIndex size = nodeIndexes[0];
		if (size == nodeIndexes[1])
		{
			DsLabelIndex *tmpNodeIndexes = new DsLabelIndex[2 + size + growStep];
			memcpy(tmpNodeIndexes, nodeIndexes, (2 + size)*sizeof(DsLabelIndex));
			delete[] nodeIndexes;
			nodeIndexes = *nodeIndexesAddress = tmpNodeIndexes;
			nodeIndexes[1] += growStep;
		}
		nodeIndexes[2 + size] = nodeIndex;
		++(nodeIndexes[0]);  // size
	}
	else
	{
		nodeIndexes = *nodeIndexesAddress = new DsLabelIndex[2 + growStep];
		nodeIndexes[0] = 1;
		nodeIndexes[1] = growStep;
		nodeIndexes[2] = nodeIndex;
	}
}

void FE_mesh_embedded_node_field::removeNode(DsLabelIndex elementIndex, DsLabelIndex nodeIndex)
{
	DsLabelIndex **nodeIndexesAddress = this->map.getAddress(elementIndex);
	DsLabelIndex *nodeIndexes = (nodeIndexesAddress) ? *nodeIndexesAddress : nullptr;
	if (!nodeIndexes)
	{
		display_message(ERROR_MESSAGE, "FE_mesh_embedded_nodes::removeNode.  "
			"Element index %d has no nodes; node index %d", elementIndex, nodeIndex);
		return;
	}
	// find nodeIndex in array, not sorted
	const DsLabelIndex size = nodeIndexes[0];
	const DsLabelIndex last = size + 1;
	// reverse order to speed up element clean-up
	for (int i = last; 1 < i; --i)
	{
		if (nodeIndexes[i] == nodeIndex)
		{
			if (size == 1)
			{
				// free only when down to zero size
				delete[] nodeIndexes;
				*nodeIndexesAddress = nullptr;
			}
			else
			{
				memmove(nodeIndexes + i, nodeIndexes + i + 1, (last - i)*sizeof(DsLabelIndex));
				--(nodeIndexes[0]);  // size
			}
			return;
		}
	}
	display_message(ERROR_MESSAGE, "FE_mesh_embedded_nodes::removeNode.  "
		"Element index %d has no entry for node index %d", elementIndex, nodeIndex);
}

DsLabelIndex FE_mesh::ElementShapeFaces::getElementFace(DsLabelIndex elementIndex, int faceNumber)
{
	// could remove following test if good arguments guaranteed
	if ((faceNumber < 0) || (faceNumber >= this->faceCount))
		return CMZN_ERROR_ARGUMENT;
	DsLabelIndex *faces = this->getElementFaces(elementIndex);
	if (!faces)
		return DS_LABEL_INDEX_INVALID;
	return faces[faceNumber];
}

FE_mesh::FE_mesh(FE_region *fe_regionIn, int dimensionIn) :
	fe_region(fe_regionIn),
	dimension(dimensionIn),
	elementShapeFacesCount(0),
	elementShapeFacesArray(0),
	elementFieldTemplateDataCount(0),
	elementFieldTemplateData(0),
	scaleFactorsCount(0),
	scaleFactorsIndexSize(0),
	lastMergedElementTemplate(0),
	parentMesh(0),
	faceMesh(0),
	changeLog(nullptr),
	element_type_node_sequence_list(0),
	definingFaces(false),
	activeElementIterators(0),
	access_count(1)
{
	this->createChangeLog();
	std::string name(this->getName());
	this->labels.setName(name + ".elements");
	for (int i = 0; i < MAXIMUM_MESH_DERIVATIVE_ORDER; ++i)
		this->fieldDerivatives[i] = FieldDerivative::createMeshDerivative(this, (i > 0) ? this->fieldDerivatives[i - 1] : nullptr);
}

FE_mesh::~FE_mesh()
{
	for (int i = 0; i < MAXIMUM_MESH_DERIVATIVE_ORDER; ++i)
	{
		this->fieldDerivatives[i]->clearOwnerPrivate();
		FieldDerivative::deaccess(this->fieldDerivatives[i]);
	}
	// safely detach from parent/face meshes
	if (this->parentMesh)
		this->parentMesh->setFaceMesh(0);
	if (this->faceMesh)
		this->faceMesh->setParentMesh(0);
	cmzn::Deaccess(this->changeLog);

	// detach objects holding non-accessed pointers to this mesh
	cmzn_elementiterator *elementIterator = this->activeElementIterators;
	while (elementIterator)
	{
		elementIterator->detachFromMesh();
		elementIterator = elementIterator->nextIterator;
	}
	for (std::set<FE_element_field_template *>::iterator iter = this->allElementfieldtemplates.begin();
		iter != this->allElementfieldtemplates.end(); ++iter)
	{
		(*iter)->detachFromMesh();
	}

	this->clear();

	if (this->embeddedNodeFields.size() > 0)
	{
		display_message(ERROR_MESSAGE, "FE_mesh::~FE_mesh.  Unexpected embedded node fields remaining");
		for (std::list<FE_mesh_embedded_node_field *>::iterator iter = this->embeddedNodeFields.begin();
			iter != this->embeddedNodeFields.end(); ++iter)
		{
			delete *iter;
		}
	}

	// destroy shared element mapping data
	for (int i = 0; i < this->elementFieldTemplateDataCount; ++i)
		delete this->elementFieldTemplateData[i];
	delete[] this->elementFieldTemplateData;
	this->elementFieldTemplateDataCount = 0;
	this->elementFieldTemplateData = 0;
}

/** Assumes called by FE_region destructor, and change notification is disabled. */
void FE_mesh::detach_from_FE_region()
{
	this->clear();
	this->fe_region = 0;
	this->parentMesh = 0;
	this->faceMesh = 0;
	this->nodeset = 0;
}

/** Called by FE_element_field_template constructor to maintain list of EFTs so
  * their mesh pointers can be cleared when mesh is destroyed.
  * @param eft  The element field template to add. Must be for this mesh.
  * @return  Result OK on success, any other error on failure. */
int FE_mesh::addElementfieldtemplate(FE_element_field_template *eft)
{
	if ((!eft) || (eft->getMesh() != this))
	{
		display_message(ERROR_MESSAGE, "FE_mesh::addElementfieldtemplate.  Invalid EFT");
		return CMZN_ERROR_ARGUMENT;
	}
	this->allElementfieldtemplates.insert(eft);
	return CMZN_OK;
}

/** Called by FE_element_field_template destructor to remove mesh's list of EFTs.
  * @param eft  The element field template to remove. Must be from this mesh.
  * @return  Result OK on success, any other error on failure. */
int FE_mesh::removeElementfieldtemplate(FE_element_field_template *eft)
{
	std::set<FE_element_field_template *>::iterator iter = this->allElementfieldtemplates.find(eft);
	if (iter == this->allElementfieldtemplates.end())
	{
		display_message(ERROR_MESSAGE, "FE_mesh::removeElementfieldtemplate.  EFT not in mesh");
		return CMZN_ERROR_ARGUMENT;
	}
	this->allElementfieldtemplates.erase(iter);
	return CMZN_OK;
}

/**
 * @return  On success accessed element field template which is "in-use" by
 * the mesh, either an existing in-use template matching eftIn or, eftIn.
 * Return 0 if eftIn does not match any EFT in use.
 * @param  eftIn  The element field template to merge. Need not be locked, but must be for this mesh.
 */
FE_element_field_template *FE_mesh::findMergedElementfieldtemplate(FE_element_field_template *eftIn)
{
	if (!eftIn)
	{
		display_message(ERROR_MESSAGE, "FE_mesh::findMergedElementfieldtemplate.  Missing template");
		return 0;
	}
	// try to find a match
	for (int i = 0; i < this->elementFieldTemplateDataCount; ++i)
	{
		if (this->elementFieldTemplateData[i])
		{
			FE_element_field_template *eft = this->elementFieldTemplateData[i]->getElementfieldtemplate();
			if (eft->matches(*eftIn))
				return eft->access();
		}
	}
	return 0;
}

/**
 * @return  On success accessed element field template which is "in-use" by
 * the mesh, either an existing in-use template matching eftIn or, eftIn
 * merged for internal use. On failure returns 0.
 * @param  eftIn  The element field template to merge. Must be locked and for this mesh.
 */
FE_element_field_template *FE_mesh::mergeElementfieldtemplate(FE_element_field_template *eftIn)
{
	if (!eftIn)
	{
		display_message(ERROR_MESSAGE, "FE_mesh::mergeElementfieldtemplate.  Missing template");
		return 0;
	}
	if (eftIn->getMesh() != this)
	{
		display_message(ERROR_MESSAGE, "FE_mesh::mergeElementfieldtemplate.  Template is not for this mesh");
		return 0;
	}
	if (eftIn->isMergedInMesh())
		return eftIn->access();
	if (!eftIn->isLocked())
	{
		display_message(ERROR_MESSAGE, "FE_mesh::mergeElementfieldtemplate.  Template is not locked");
		return 0;
	}
	// try to find a match
	int firstUnusedIndex = -1;
	for (int i = 0; i < this->elementFieldTemplateDataCount; ++i)
	{
		if (this->elementFieldTemplateData[i])
		{
			FE_element_field_template *eft = this->elementFieldTemplateData[i]->getElementfieldtemplate();
			if (eft->matches(*eftIn))
				return eft->access();
		}
		else if (firstUnusedIndex < 0)
		{
			firstUnusedIndex = i;
		}
	}
	// merge eftIn
	if (firstUnusedIndex < 0)
	{
		// reallocate
		firstUnusedIndex = this->elementFieldTemplateDataCount;
		FE_mesh_element_field_template_data **tmp = new FE_mesh_element_field_template_data*[this->elementFieldTemplateDataCount + 1];
		memcpy(tmp, this->elementFieldTemplateData, this->elementFieldTemplateDataCount*sizeof(FE_mesh_element_field_template_data*));
		delete[] this->elementFieldTemplateData;
		this->elementFieldTemplateData = tmp;
		++this->elementFieldTemplateDataCount;
	}
	this->elementFieldTemplateData[firstUnusedIndex] = new FE_mesh_element_field_template_data(eftIn, &(this->labels));
	eftIn->setIndexInMesh(this, firstUnusedIndex);
	return eftIn->access();
}

/**
 * @return  A new blank field template for this mesh with one access count to
 * be consumed by caller.
 */
FE_mesh_field_template *FE_mesh::createBlankMeshFieldTemplate()
{
	FE_mesh_field_template *meshFieldTemplate = new FE_mesh_field_template(this);
	this->meshFieldTemplates.push_back(meshFieldTemplate);
	return meshFieldTemplate;
}

/** @return  Accessed clone of the supplied template suitable for modifying. Must be from this
  * mesh. Returns 0 if failed or source is not from this mesh. */
FE_mesh_field_template *FE_mesh::cloneMeshFieldTemplate(const FE_mesh_field_template *source)
{
	if (!((source) && (this == source->getMesh())))
	{
		display_message(ERROR_MESSAGE, "FE_mesh::removeFieldTemplate.  Invalid argument");
		return 0;
	}
	FE_mesh_field_template *meshFieldTemplate = new FE_mesh_field_template(*source);
	this->meshFieldTemplates.push_back(meshFieldTemplate);
	return meshFieldTemplate;
}

/**
 * @return  Existing or new blank field template for this mesh with incremented
 * access count to be consumed by caller.
 */
FE_mesh_field_template *FE_mesh::getOrCreateBlankMeshFieldTemplate()
{
	for (std::list<FE_mesh_field_template*>::iterator iter = this->meshFieldTemplates.begin(); iter != this->meshFieldTemplates.end(); ++iter)
	{
		if ((*iter)->isBlank())
			return (*iter)->access();
	}
	return this->createBlankMeshFieldTemplate();
}

/** Only to be called by ~FE_mesh_field_template */
void FE_mesh::removeMeshFieldTemplate(FE_mesh_field_template *meshFieldTemplate)
{
	for (std::list<FE_mesh_field_template*>::iterator iter = this->meshFieldTemplates.begin();
		iter != this->meshFieldTemplates.end(); ++iter)
	{
		if (*iter == meshFieldTemplate)
		{
			this->meshFieldTemplates.erase(iter);
			return;
		}
	}
	display_message(ERROR_MESSAGE, "FE_mesh::removeFieldTemplate.  Field template not found");
}

FE_mesh_embedded_node_field *FE_mesh::addEmbeddedNodeField(FE_field *field, FE_nodeset *nodeset)
{
	if ((field) && (field->get_FE_region() == this->fe_region) &&
		(nodeset) && (nodeset->get_FE_region() == this->fe_region))
	{
		FE_mesh_embedded_node_field *embeddedNodeField = new FE_mesh_embedded_node_field(field, nodeset);
		this->embeddedNodeFields.push_back(embeddedNodeField);
		return embeddedNodeField;
	}
	display_message(ERROR_MESSAGE, "FE_mesh::addEmbeddedNodeField.  Invalid arguments");
	return nullptr;
}

void FE_mesh::removeEmbeddedNodeField(FE_mesh_embedded_node_field*& embeddedNodeField)
{
	std::list<FE_mesh_embedded_node_field *>::iterator iter = std::find(this->embeddedNodeFields.begin(), this->embeddedNodeFields.end(), embeddedNodeField);
	if (iter != this->embeddedNodeFields.end())
	{
		this->embeddedNodeFields.erase(iter);
		delete embeddedNodeField;
		embeddedNodeField = nullptr;
	}
	else
	{
		display_message(ERROR_MESSAGE, "FE_mesh::addEmbeddedNodeField.  Embedded node field not found");
	}
}


/** @return  True if fields defined identically for the two elements
  * @param elementIndex1, elementIndex2  Element indexes to compare. Not checked, must be valid. */
bool FE_mesh::equivalentFieldsInElements(DsLabelIndex elementIndex1, DsLabelIndex elementIndex2) const
{
	for (std::list<FE_mesh_field_template*>::const_iterator iter = this->meshFieldTemplates.begin();
		iter != this->meshFieldTemplates.end(); ++iter)
	{
		if ((*iter)->getElementEFTIndex(elementIndex1) != (*iter)->getElementEFTIndex(elementIndex1))
			return false;
	}
	return true;
}

/**
 * Call this to mark element with the supplied change.
 * Notifies change to clients of FE_region.
 * @param change  Logical OR of values from enum DsLabelChangeType
 */
void FE_mesh::elementChange(DsLabelIndex elementIndex, int change)
{
	if (this->fe_region && this->changeLog)
	{
		this->changeLog->setIndexChange(elementIndex, change);
		this->fe_region->FE_region_change();
		this->fe_region->update();
	}
}

/**
 * Call this to mark element with the supplied change and related change to field.
 * Notifies change to clients of FE_region.
 * @param change  Logical OR of values from enum DsLabelChangeType
 */
void FE_mesh::elementFieldChange(DsLabelIndex elementIndex, int change, FE_field *fe_field)
{
	if (this->fe_region && this->changeLog)
	{
		this->changeLog->setIndexChange(elementIndex, change);
		this->fe_region->FE_field_change(fe_field, CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field));
		this->fe_region->update();
	}
}
/**
 * Call this to mark element with the supplied change.
 * Conservatively marks all fields as changed too.
 * Notifies change to clients of FE_region.
 * @param change  Logical OR of values from enum DsLabelChangeType
 */
void FE_mesh::elementAllFieldChange(DsLabelIndex elementIndex, int change)
{
	if (this->fe_region && this->changeLog)
	{
		this->changeLog->setIndexChange(elementIndex, change);
		this->fe_region->FE_field_all_change(CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field));
		this->fe_region->update();
	}
}

namespace {

int FE_field_clear_on_mesh_iterator(FE_field *field, void *mesh_void)
{
	field->clearMeshFieldData(static_cast<FE_mesh *>(mesh_void));
	return 1;
}

}

// remove field definitions on this mesh, and element nodes and scale factors
// call only from clear() or detach_from_FE_region
// Must have fe_region member for this to do anything.
// Changes to fields are recorded but no FE_region update is made
void FE_mesh::clearElementFieldData()
{
	if (this->fe_region)
	{
		// remove field definitions on this mesh. This *should* clear all mesh field templates.
		FE_region_for_each_FE_field(this->fe_region, FE_field_clear_on_mesh_iterator, static_cast<void *>(this));
	}
	if (this->meshFieldTemplates.size() != 0)
		display_message(ERROR_MESSAGE, "FE_mesh::clearElementFieldData.  Not all mesh field templates removed");
	// clear element-varying nodes, scale factors held with element field templates
	// can't remove eft objects as may be present in element templates
	for (int i = 0; i < this->elementFieldTemplateDataCount; ++i)
	{
		if (this->elementFieldTemplateData[i])
			this->elementFieldTemplateData[i]->clearAllElementVaryingData();
	}
}

/** @return  Name of mesh, not to be freed. Currently restricted to
  * "mesh1d", "mesh2d" or "mesh3d", based on dimension. */
const char *FE_mesh::getName() const
{
	if (1 == this->dimension)
		return "mesh1d";
	if (2 == this->dimension)
		return "mesh2d";
	if (3 == this->dimension)
		return "mesh3d";
	return 0;
}

// Only to be called by ~FE_mesh, FE_region_clear, or when all elements already removed
// to reclaim memory in labels and mapped arrays
void FE_mesh::clear()
{
	this->clearElementFieldData();
	if (0 < this->labels.getSize())
	{
		const DsLabelIndex indexLimit = this->labels.getIndexSize();
		if (this->parentMesh)
		{
			// fast cleanup of dynamically allocated parent arrays
			for (DsLabelIndex index = 0; index < indexLimit; ++index)
			{
				DsLabelIndex **parentsArrayAddress = this->parents.getAddress(index);
				if (parentsArrayAddress && *parentsArrayAddress)
					delete[] *parentsArrayAddress;
			}
		}
		// clear embedding of nodes in this mesh
		for (int i = 0; i < 2; ++i)
		{
			// this is not very efficient
			FE_nodeset_clear_embedded_locations(this->fe_region->nodesets[i],
				this->fe_region->fe_field_list, /*hostMesh*/this);
		}
		cmzn_element *element;
		for (DsLabelIndex index = 0; index < indexLimit; ++index)
		{
			if (this->fe_elements.getValue(index, element) && (element))
			{
				element->invalidate();
				cmzn_element::deaccess(element);
			}
		}
		this->changeLog->setAllChange(DS_LABEL_CHANGE_TYPE_REMOVE);
	}
	this->fe_elements.clear();

	for (int i = 0; i < this->elementShapeFacesCount; ++i)
		delete this->elementShapeFacesArray[i];
	delete[] this->elementShapeFacesArray;
	this->elementShapeFacesCount = 0;
	this->elementShapeFacesArray = 0;
	this->elementShapeMap.clear();
	// dynamic parent arrays have been freed above
	this->parents.clear();

	// clear scale factors and associated maps
	this->scaleFactors.clear();
	this->scaleFactorUsageCounts.clear();
	this->globalScaleFactorsIndex.clear();
	this->nodeScaleFactorsIndex.clear();
	this->scaleFactorsCount = 0;
	this->scaleFactorsIndexSize = 0;

	this->labels.clear();
}

/** Private: can be used to clear cached changes */
void FE_mesh::createChangeLog()
{
	cmzn::Deaccess(this->changeLog);
	this->changeLog = DsLabelsChangeLog::create(&this->labels);
	if (!this->changeLog)
		display_message(ERROR_MESSAGE, "FE_mesh::createChangeLog.  Failed to create change log");
}

DsLabelsChangeLog *FE_mesh::extractChangeLog()
{
	// take access count of changelog when extracting
	DsLabelsChangeLog *returnChangeLog = this->changeLog;
	this->changeLog = nullptr;
	this->lastMergedElementTemplate = 0; // ensures field notifications are recorded if template is merged again
	this->createChangeLog();
	return returnChangeLog;
}

/**
 * Set the element shape for the element at index.
 * Logs no object changes - assume done by callers
 * @param index  The index of the element in the mesh
 * @param element_shape  The shape to set; must be of same dimension as mesh.
 * @return  ElementShapeFaces for element with index, or 0 if failed.
 */
FE_mesh::ElementShapeFaces *FE_mesh::setElementShape(DsLabelIndex elementIndex, FE_element_shape *element_shape)
{
	if ((elementIndex < 0) || (get_FE_element_shape_dimension(element_shape) != this->dimension))
		return 0;
	ElementShapeFaces *currentElementShapeFaces = this->getElementShapeFaces(elementIndex);
	if (currentElementShapeFaces)
	{
		if (currentElementShapeFaces->getElementShape() == element_shape)
			return currentElementShapeFaces;
		// should check usage/efficiency for multiple changes, ensure element_shape is not degenerate.
		if (this->parentMesh)
			this->clearElementParents(elementIndex);
		if (this->faceMesh)
			this->clearElementFaces(elementIndex);
	}
	int shapeIndex = 0;
	while ((shapeIndex < this->elementShapeFacesCount) &&
			(this->elementShapeFacesArray[shapeIndex]->getElementShape() != element_shape))
		++shapeIndex;
	if (shapeIndex == this->elementShapeFacesCount)
	{
		if (1 == this->elementShapeFacesCount)
		{
			// must now store per-element shape
			if (!this->elementShapeMap.setValues(0, this->labels.getIndexSize(), 0))
			{
				display_message(ERROR_MESSAGE, "FE_mesh::setElementShape  Failed to make per-element shape map");
				return 0;
			}
		}
		ElementShapeFaces *newElementShapeFaces = new ElementShapeFaces(&this->labels, element_shape);
		if (!newElementShapeFaces)
			return 0;
		ElementShapeFaces **tempElementShapeFaces = new ElementShapeFaces*[this->elementShapeFacesCount + 1];
		if (!tempElementShapeFaces)
		{
			delete newElementShapeFaces;
			return 0;
		}
		for (int i = 0; i < this->elementShapeFacesCount; ++i)
			tempElementShapeFaces[i] = this->elementShapeFacesArray[i];
		tempElementShapeFaces[shapeIndex] = newElementShapeFaces;
		delete[] this->elementShapeFacesArray;
		this->elementShapeFacesArray = tempElementShapeFaces;
		++this->elementShapeFacesCount;
	}
	if ((this->elementShapeFacesCount > 1) &&
			(!this->elementShapeMap.setValue(elementIndex, shapeIndex)))
		return 0;
	return this->elementShapeFacesArray[shapeIndex];
}

/** @param elementTemplate  Not checked, assume valid. */
bool FE_mesh::setElementShapeFromElementTemplate(DsLabelIndex elementIndex, FE_element_template *elementTemplate)
{
	FE_element_shape *elementShape = elementTemplate->getElementShape();
	if (!elementShape)
		return true;
	// GRC make more efficient by caching shapeIndex
	return 0 != this->setElementShape(elementIndex, elementShape);
}

/**
 * Define fields on element as defined in element template.
 * Prerequisite: element_template must have been validated and all EFTs 'made global'
 * Assumes FE_region change cache is on.
 * @param elementIndex  Not checked; assume valid.
 * @param elementTemplate  Not checked, assume valid.
 */
bool FE_mesh::mergeFieldsFromElementTemplate(DsLabelIndex elementIndex, FE_element_template *elementTemplate)
{
	const int fieldCount = elementTemplate->fieldCount;
	if ((elementTemplate != this->lastMergedElementTemplate) && (this->fe_region))
	{
		// record changes to fields, but don't check for FE_region update as caching is assumed
		for (int f = 0; f < fieldCount; ++f)
			this->fe_region->FE_field_change(
				elementTemplate->fields[f]->getField(), CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field));
		this->lastMergedElementTemplate = elementTemplate;
	}

	// cache mesh field template for each field component, clear once component is merged
	for (int f = 0; f < fieldCount; ++f)
	{
		FE_element_template::FE_field_data *elementFieldData = elementTemplate->fields[f];
		FE_field *field = elementFieldData->getField();
		FE_mesh_field_data *meshFieldData = field->getMeshFieldData(this);
		if (elementFieldData->isUndefine())
		{
			if (!meshFieldData)
			{
				// clear following to simplify merge algorithm below
				for (int c = 0; c < elementFieldData->componentCount; ++c)
					elementFieldData->setCacheComponentMeshFieldTemplate(c, 0);
				continue;
			}
		}
		else if (!meshFieldData)
		{
			meshFieldData = field->createMeshFieldData(this);
			if (!meshFieldData)
			{
				display_message(ERROR_MESSAGE, "Mesh mergeFieldsFromElementTemplate.  Failed to create mesh field data");
				return false;
			}
		}
		for (int c = 0; c < elementFieldData->componentCount; ++c)
		{
			FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
			elementFieldData->setCacheComponentMeshFieldTemplate(c, mft);
		}
	}
	// now perform merge, trying to keep sharing mesh field templates between field components where possible
	for (int f = 0; f < fieldCount; ++f)
	{
		FE_element_template::FE_field_data *elementFieldData = elementTemplate->fields[f];
		for (int c = 0; c < elementFieldData->componentCount; ++c)
		{
			FE_mesh_field_template *mft = elementFieldData->getCacheComponentMeshFieldTemplate(c);
			if (!mft)
				continue; // already merged
			FE_element_field_template *eft = elementFieldData->getComponentElementfieldtemplate(c);
			FE_element_field_template *old_eft = mft->getElementfieldtemplate(elementIndex);
			int eft_mft_match_count = 1;
			int c2 = c + 1;
			for (int f2 = f; f2 < fieldCount; ++f2)
			{
				FE_element_template::FE_field_data *elementFieldData2 = elementTemplate->fields[f2];
				for (; c2 < elementFieldData2->componentCount; ++c2)
				{
					if ((elementFieldData2->getComponentElementfieldtemplate(c2) == eft)
						&& (elementFieldData2->getCacheComponentMeshFieldTemplate(c2) == mft))
						++eft_mft_match_count;
				}
				c2 = 0;
			}
			FE_mesh_field_template *new_mft = mft;
			if (eft_mft_match_count < mft->getUsageCount())
			{
				// only some fields using mesh field template changing eft, hence clone, modify and set in fields
				new_mft = this->cloneMeshFieldTemplate(mft);
				if (!new_mft)
				{
					display_message(ERROR_MESSAGE, "Mesh mergeFieldsFromElementTemplate.  Failed to clone field template");
					return false;
				}
			}
			if (eft != old_eft)
			{
				if (!new_mft->setElementfieldtemplate(elementIndex, eft))
				{
					display_message(ERROR_MESSAGE, "Mesh mergeFieldsFromElementTemplate.  Failed to set element field template");
					return false;
				}
			}
			c2 = c;
			for (int f2 = f; f2 < fieldCount; ++f2)
			{
				FE_element_template::FE_field_data *elementFieldData2 = elementTemplate->fields[f2];
				FE_field *field2 = elementFieldData2->getField();
				FE_mesh_field_data *meshFieldData2 = field2->getMeshFieldData(this);
				for (; c2 < elementFieldData2->componentCount; ++c2)
				{
					if ((elementFieldData2->getComponentElementfieldtemplate(c2) == eft)
						&& (elementFieldData2->getCacheComponentMeshFieldTemplate(c2) == mft))
					{
						if (eft != old_eft)
						{
							if ((old_eft) && old_eft->hasElementDOFs())
								meshFieldData2->clearComponentElementData(c2, elementIndex);
							// GRC could allocate new per-element data here, otherwise assuming done on demand
							// if we change it here, must change it for the above case of modifying existing mft
						}
						if (new_mft != mft)
							meshFieldData2->setComponentMeshfieldtemplate(c2, new_mft);
						elementFieldData2->setCacheComponentMeshFieldTemplate(c2, 0);
					}
				}
				c2 = 0;
			}
			if (new_mft != mft)
				FE_mesh_field_template::deaccess(new_mft);
		}
	}
	return true;
}

struct FE_element_field_info_check_field_node_value_labels_data
{
	struct FE_field *field;
	struct FE_region *target_fe_region;
};

void FE_mesh::list_btree_statistics()
{
	if (this->labels.getSize() > 0)
	{
		display_message(INFORMATION_MESSAGE, "%d-D elements:\n", this->dimension);
		this->labels.list_storage_details();
	}
}

/** Remove iterator from linked list in this mesh */
void FE_mesh::removeElementiterator(cmzn_elementiterator *iterator)
{
	if (iterator == this->activeElementIterators)
		this->activeElementIterators = iterator->nextIterator;
	else
	{
		cmzn_elementiterator *prevIterator = this->activeElementIterators;
		while (prevIterator && (prevIterator->nextIterator != iterator))
			prevIterator = prevIterator->nextIterator;
		if (prevIterator)
			prevIterator->nextIterator = iterator->nextIterator;
		else
			display_message(ERROR_MESSAGE, "FE_mesh::removeElementiterator.  Iterator not in linked list");
	}
	iterator->nextIterator = 0;
}

/**
 * Create an element iterator object for iterating through the elements of the
 * mesh. The iterator initially points at the position before the first element.
 * @param labelsGroup  Optional group to iterate over.
 * @return  Handle to element_iterator at position before first, or NULL if error.
 */
cmzn_elementiterator *FE_mesh::createElementiterator(DsLabelsGroup *labelsGroup)
{
	DsLabelIterator *labelIterator = labelsGroup ? labelsGroup->createLabelIterator() : this->labels.createLabelIterator();
	if (!labelIterator)
		return 0;
	cmzn_elementiterator *iterator = new cmzn_elementiterator(this, labelIterator);
	if (iterator)
	{
		iterator->nextIterator = this->activeElementIterators;
		this->activeElementIterators = iterator;
	}
	else
		cmzn::Deaccess(labelIterator);
	return iterator;
}

cmzn_element *FE_mesh::get_first_FE_element_that(
	LIST_CONDITIONAL_FUNCTION(cmzn_element) *conditional_function, void *user_data_void)
{
	DsLabelIterator *iter = this->labels.createLabelIterator();
	if (!iter)
		return 0;
	DsLabelIndex elementIndex;
	cmzn_element *element = 0;
	while ((elementIndex = iter->nextIndex()) != DS_LABEL_INDEX_INVALID)
	{
		element = this->getElement(elementIndex);
		if (!element)
		{
			display_message(ERROR_MESSAGE, "FE_mesh::for_each_FE_element.  No element at index");
			break;
		}
		if ((!conditional_function) || conditional_function(element, user_data_void))
			break;
	}
	cmzn::Deaccess(iter);
	if (elementIndex >= 0)
		return element;
	return 0;
}

int FE_mesh::for_each_FE_element(
	LIST_ITERATOR_FUNCTION(cmzn_element) iterator_function, void *user_data_void)
{
	DsLabelIterator *iter = this->labels.createLabelIterator();
	if (!iter)
		return 0;
	int return_code = 1;
	DsLabelIndex elementIndex;
	cmzn_element *element;
	while ((elementIndex = iter->nextIndex()) != DS_LABEL_INDEX_INVALID)
	{
		element = this->getElement(elementIndex);
		if (!element)
		{
			display_message(ERROR_MESSAGE, "FE_mesh::for_each_FE_element.  No element at index");
			return_code = 0;
			break;
		}
		if (!iterator_function(element, user_data_void))
		{
			return_code = 0;
			break;
		}
	}
	cmzn::Deaccess(iter);
	return return_code;
}

DsLabelsGroup *FE_mesh::createLabelsGroup()
{
	return DsLabelsGroup::create(&this->labels); // GRC dodgy taking address here
}

int FE_mesh::setElementIdentifier(DsLabelIndex elementIndex, int identifier)
{
	if ((this->getElementIdentifier(elementIndex) >= 0) && (identifier >= 0))
	{
		int return_code = this->labels.setIdentifier(elementIndex, identifier);
		if (return_code == CMZN_OK)
			this->elementChange(elementIndex, DS_LABEL_CHANGE_TYPE_IDENTIFIER);
		else if (return_code == CMZN_ERROR_ALREADY_EXISTS)
			display_message(ERROR_MESSAGE, "FE_mesh::setElementIdentifier.  Identifier %d is already used in %d-D mesh",
				identifier, this->dimension);
		else
			display_message(ERROR_MESSAGE, "FE_mesh::setElementIdentifier.  Failed to set label identifier");
		return return_code;
	}
	display_message(ERROR_MESSAGE, "FE_mesh::setElementIdentifier.  Invalid argument(s)");
	return CMZN_ERROR_ARGUMENT;
}

/** @param element_shape  Element shape, must match mesh dimension */
FE_element_template *FE_mesh::create_FE_element_template(FE_element_shape *element_shape)
{
	if ((element_shape) && (get_FE_element_shape_dimension(element_shape) != this->dimension))
		return 0;
	return new FE_element_template(this, element_shape);
}

/** Clean up element label and objects. Called by element create functions when they fail part way. */
void FE_mesh::destroyElementPrivate(DsLabelIndex elementIndex)
{
	cmzn_element **elementAddress = this->fe_elements.getAddress(elementIndex);
	if (elementAddress && (*elementAddress))
	{
		(*elementAddress)->invalidate();
		cmzn_element::deaccess(*elementAddress);
	}
	this->labels.removeLabel(elementIndex);
}

/**
 * Create a blank element with the given identifier and no shape.
 * Private. Does not perform change recording or notification.
 * @param  identifier  Identifier for the new element, must be non-negative
 * and unique in mesh, or -1 to generate an arbitrary identifier.
 * @return  Index of new element, or DS_LABEL_INDEX_INVALID if failed.
 */
DsLabelIndex FE_mesh::createElement(DsLabelIdentifier identifier)
{
	if (identifier < -1)
		return DS_LABEL_INDEX_INVALID;
	DsLabelIndex elementIndex = (identifier < 0) ? this->labels.createLabel() : this->labels.createLabel(identifier);
	if (elementIndex >= 0)
	{
		cmzn_element *element = new cmzn_element(this, elementIndex);
		if (!((element) && this->fe_elements.setValue(elementIndex, element)))
		{
			cmzn_element::deaccess(element);
			this->labels.removeLabel(elementIndex);
			return DS_LABEL_INDEX_INVALID;
		}
	}
	return elementIndex;
}

/**
 * Create a blank element with the given identifier and no shape.
 * Private. Does not perform change recording or notification.
 * @param  identifier  Identifier for the new element, must be non-negative
 * and unique in mesh, or -1 to generate an arbitrary identifier.
 * @return  Non-accessed element, or 0 if failed.
 */
cmzn_element *FE_mesh::createElementObject(DsLabelIdentifier identifier)
{
	if (identifier < -1)
		return 0;
	DsLabelIndex elementIndex = (identifier < 0) ? this->labels.createLabel() : this->labels.createLabel(identifier);
	if (elementIndex < 0)
		return 0;
	cmzn_element *element = new cmzn_element(this, elementIndex);
	if (!((element) && this->fe_elements.setValue(elementIndex, element)))
	{
		cmzn_element::deaccess(element);
		this->labels.removeLabel(elementIndex);
	}
	// record element added
	this->changeLog->setIndexChange(elementIndex, DS_LABEL_CHANGE_TYPE_ADD);
	return element;
}

/**
 * Checks the element_template is compatible with mesh & that there is no
 * existing element of supplied identifier, then creates element of that
 * identifier as a copy of element_template and adds it to the mesh.
 * Note: assumes FE_region change cache is on.
 *
 * @param identifier  Non-negative integer identifier of new element, or -1 to
 * automatically generate (starting at 1). Fails if supplied identifier already
 * used by an existing element.
 * @return  Accessed element, or 0 on error.
 */
cmzn_element *FE_mesh::create_FE_element(int identifier, FE_element_template *elementTemplate)
{
	if ((-1 <= identifier) && (elementTemplate) && elementTemplate->isValidated())
	{
		if (elementTemplate->mesh == this)
		{
			cmzn_element *element = this->createElementObject(identifier);
			if (element)
			{
				const DsLabelIndex elementIndex = element->getIndex();
				if (this->setElementShapeFromElementTemplate(elementIndex, elementTemplate)
					&& this->mergeFieldsFromElementTemplate(elementIndex, elementTemplate))
				{
					element->access();
					return element;
				}
				else
				{
					display_message(ERROR_MESSAGE, "FE_mesh::create_FE_element.  Failed to set element shape or fields.");
					this->destroyElementPrivate(elementIndex);
					element = 0;
				}
			}
			else
			{
				if (this->labels.findLabelByIdentifier(identifier) >= 0)
					display_message(ERROR_MESSAGE, "FE_mesh::create_FE_element.  Identifier %d is already used in %d-D mesh.",
						identifier, this->dimension);
				else
					display_message(ERROR_MESSAGE, "FE_mesh::create_FE_element.  Could not create element");
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"FE_mesh::create_FE_element.  Element template is incompatible with mesh");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "FE_mesh::create_FE_element.  Invalid arguments");
	}
	return 0;
}

/**
 * Convenience function returning an existing element with the identifier
 * from the mesh, or if none found or if identifier is -1, a new element with
 * with the identifier (or the first available identifier if -1), and with the
 * supplied shape (which can be none for an unspecified shape intended to be
 * found on merging.
 * Note: assumes FE_region change cache is on.
 * @return  Accessed element, or 0 on error.
 */
cmzn_element *FE_mesh::get_or_create_FE_element_with_identifier(int identifier,
	struct FE_element_shape *element_shape)
{
	cmzn_element *element = 0;
	if ((-1 <= identifier) && ((!element_shape) ||
		(get_FE_element_shape_dimension(element_shape) == this->dimension)))
	{
		if (identifier >= 0)
		{
			element = this->findElementByIdentifier(identifier);
			if (element)
				return element->access();
		}
		element = this->createElementObject(identifier);
		if (element)
		{
			if ((!(element_shape)) || (0 != this->setElementShape(element->getIndex(), element_shape)))
				return element->access();
			this->destroyElementPrivate(element->getIndex());
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_mesh::get_or_create_FE_element_with_identifier.  Invalid argument(s)");
	}
	return 0;
}

/**
 * Redefine fields on destination with definition from element template, and
 * optionally change element shape.
 * Note: assumes FE_region change cache is on.
 */
int FE_mesh::merge_FE_element_template(cmzn_element *destination, FE_element_template *elementTemplate)
{
	if (elementTemplate && (elementTemplate->getMesh() == this) && elementTemplate->isValidated())
	{
		const DsLabelIndex elementIndex = get_FE_element_index(destination);
		const bool shapeChange = (elementTemplate->getElementShape()) &&
			(elementTemplate->getElementShape() != this->getElementShape(elementIndex));
		if (((!shapeChange) || (this->setElementShapeFromElementTemplate(elementIndex, elementTemplate)))
			&& this->mergeFieldsFromElementTemplate(elementIndex, elementTemplate))
		{
			int change = DS_LABEL_CHANGE_TYPE_RELATED;
			if (shapeChange)
				change |= DS_LABEL_CHANGE_TYPE_DEFINITION;
			this->changeLog->setIndexChange(elementIndex, change);
			return CMZN_OK;
		}
		else
		{
			display_message(ERROR_MESSAGE, "FE_mesh::merge_FE_element_template.  Failed.");
			return CMZN_ERROR_GENERAL;
		}
	}
	display_message(ERROR_MESSAGE, "FE_mesh::merge_FE_element_template.  Invalid arguments");
	return CMZN_ERROR_ARGUMENT;
}

/** Add parent index to end of list of parents for element.
 * Private: assumes both indexes are >= 0 */
int FE_mesh::addElementParent(DsLabelIndex elementIndex, DsLabelIndex parentIndex)
{
	DsLabelIndex *oldParentsArray = 0;
	int parentsCount;
	if (this->parents.getValue(elementIndex, oldParentsArray) && oldParentsArray)
		parentsCount = oldParentsArray[0];
	else
		parentsCount = 0;
	++parentsCount;
	DsLabelIndex *parentsArray = new DsLabelIndex[parentsCount + 1]; // with one extra space for count
	if (!parentsArray)
		return CMZN_ERROR_MEMORY;
	parentsArray[0] = parentsCount;
	if (oldParentsArray)
	{
		for (int i = 1; i < parentsCount; ++i)
			parentsArray[i] = oldParentsArray[i];
		delete[] oldParentsArray;
	}
	parentsArray[parentsCount] = parentIndex;
	if (this->parents.setValue(elementIndex, parentsArray))
		return CMZN_OK;
	return CMZN_ERROR_MEMORY;
}

/** Removes first instance of parent index from list of parents for element.
 * Private: assumes both indexes are >= 0 */
int FE_mesh::removeElementParent(DsLabelIndex elementIndex, DsLabelIndex parentIndex)
{
	DsLabelIndex *parentsArray = 0;
	if (this->parents.getValue(elementIndex, parentsArray) && parentsArray)
	{
		const int parentsCount = parentsArray[0];
		for (int i = 1; i <= parentsCount; ++i)
			if (parentsArray[i] == parentIndex)
			{
				if ((--parentsArray[0]) == 0)
				{
					delete[] parentsArray;
					if (this->parents.setValue(elementIndex, 0))
						return CMZN_OK;
					return CMZN_ERROR_GENERAL;
				}
				for (int j = i; j < parentsCount; ++j)
					parentsArray[j] = parentsArray[j + 1];
				return CMZN_OK;
			}
	}
	return CMZN_ERROR_NOT_FOUND;
}

/** Remove all storage for parents for element. Safe version for continued use of
  * region: removes this element from faces of parents, and records their change.
  * Private: assumes element index is >= 0. Call only if mesh has parent mesh.
  * Note: assumes FE_region change cache is on. */
void FE_mesh::clearElementParents(DsLabelIndex elementIndex)
{
	// remove element from all parents; mark parent elements as DEFINITION_CHANGED
	int parentsCount;
	const DsLabelIndex *parents;
	while (0 < (parentsCount = this->getElementParents(elementIndex, parents)))
	{
		const int faceNumber = this->parentMesh->getElementFaceNumber(parents[0], elementIndex);
		if (CMZN_OK != this->parentMesh->setElementFace(parents[0], faceNumber, DS_LABEL_INDEX_INVALID))
			return;
	}
}

/** Clear all faces of element. Remove any face elements without other parents from face mesh.
 * Private: assumes element index is >= 0. Call only if mesh has faceMesh */
void FE_mesh::clearElementFaces(DsLabelIndex elementIndex)
{
	ElementShapeFaces *elementShapeFaces = this->getElementShapeFaces(elementIndex);
	if (!elementShapeFaces)
	{
		display_message(ERROR_MESSAGE, "FE_mesh::clearElementFaces.  Missing ElementShapeFaces");
		return;
	}
	// remove faces used by no other parent elements
	DsLabelIndex *faces = elementShapeFaces->getElementFaces(elementIndex);
	if (!faces)
		return;
	const int faceCount = elementShapeFaces->getFaceCount();
	for (int i = 0; i < faceCount; ++i)
	{
		DsLabelIndex faceIndex = faces[i]; // must put in local variable since cleared by setElementFace
		if (faceIndex >= 0)
		{
			// don't notify parent modified since only called from removeElementPrivate or on shape change
			this->faceMesh->removeElementParent(faceIndex, elementIndex);
			faces[i] = DS_LABEL_INDEX_INVALID;
			const DsLabelIndex *parents;
			if (0 == this->faceMesh->getElementParents(faceIndex, parents))
				this->faceMesh->removeElementPrivate(faceIndex);
		}
	}
	elementShapeFaces->destroyElementFaces(elementIndex);
}

// set index of face element (from face mesh)
int FE_mesh::setElementFace(DsLabelIndex elementIndex, int faceNumber, DsLabelIndex faceIndex)
{
	if ((elementIndex < 0) || (!this->faceMesh))
		return CMZN_ERROR_ARGUMENT;
	ElementShapeFaces *elementShapeFaces = this->getElementShapeFaces(elementIndex);
	if (!elementShapeFaces)
		return CMZN_ERROR_GENERAL;
	if ((faceNumber < 0) || (faceNumber >= elementShapeFaces->getFaceCount()))
		return CMZN_ERROR_ARGUMENT;
	// could in future handle special case of setting invalid face when no faces currently
	DsLabelIndex *faces = elementShapeFaces->getOrCreateElementFaces(elementIndex);
	if (!faces)
		return CMZN_ERROR_MEMORY;
	const DsLabelIndex oldFaceIndex = faces[faceNumber];
	if (oldFaceIndex != faceIndex)
	{
		faces[faceNumber] = faceIndex;
		if (oldFaceIndex >= 0)
			this->faceMesh->removeElementParent(oldFaceIndex, elementIndex);
		if (faceIndex >= 0)
			return this->faceMesh->addElementParent(faceIndex, elementIndex);
		this->changeLog->setIndexChange(elementIndex, DS_LABEL_CHANGE_TYPE_DEFINITION);
	}
	return CMZN_OK;
}

/** return the face number of faceIndex in elementIndex or -1 if not a face */
int FE_mesh::getElementFaceNumber(DsLabelIndex elementIndex, DsLabelIndex faceIndex) const
{
	const ElementShapeFaces *elementShapeFaces = this->getElementShapeFaces(elementIndex);
	if (elementShapeFaces)
	{
		const DsLabelIndex *faces = elementShapeFaces->getElementFaces(elementIndex);
		if (faces)
			for (int faceNumber = elementShapeFaces->getFaceCount() - 1; faceNumber >= 0; --faceNumber)
				if (faces[faceNumber] == faceIndex)
					return faceNumber;
	}
	return -1;
}

/** Note this is potentially expensive if there are a lot of EFTs in use.
  * @return  True if element references any node in the node group. */
bool FE_mesh::elementHasNodeInGroup(DsLabelIndex elementIndex, const DsLabelsGroup& nodeLabelsGroup) const
{
	for (int i = 0; i < this->elementFieldTemplateDataCount; ++i)
	{
		const FE_mesh_element_field_template_data *eftData = this->elementFieldTemplateData[i];
		if (!eftData)
			continue;
		const int localNodeCount = eftData->getElementfieldtemplate()->getNumberOfLocalNodes();
		if (localNodeCount)
		{
			const DsLabelIndex *nodeIndexes = eftData->getElementNodeIndexes(elementIndex);
			if (nodeIndexes)
			{
				for (int n = 0; n < localNodeCount; ++n)
					if (nodeLabelsGroup.hasIndex(nodeIndexes[n]))
						return true;
			}
		}
	}
	return false;
}

/** Ensure all nodes used by element are in the group.
  * Does not handle nodes inherited from parent elements.
  * Note this is potentially expensive if there are a lot of EFTs in use.
  * @return  True on success, false on failure. */
bool FE_mesh::addElementNodesToGroup(DsLabelIndex elementIndex, DsLabelsGroup& nodeLabelsGroup)
{
	for (int i = 0; i < this->elementFieldTemplateDataCount; ++i)
	{
		const FE_mesh_element_field_template_data *eftData = this->elementFieldTemplateData[i];
		if (!eftData)
			continue;
		const int localNodeCount = eftData->getElementfieldtemplate()->getNumberOfLocalNodes();
		if (localNodeCount)
		{
			const DsLabelIndex *nodeIndexes = eftData->getElementNodeIndexes(elementIndex);
			if (nodeIndexes)
				for (int n = 0; n < localNodeCount; ++n)
					if (nodeIndexes[n] >= 0)
					{
						const int result = nodeLabelsGroup.setIndex(nodeIndexes[n], true);
						if ((result != CMZN_OK) && (result != CMZN_ERROR_ALREADY_EXISTS))
							return false;
					}
		}
	}
	return true;
}

/** Ensure all nodes used by element are in the group.
  * Does not handle nodes inherited from parent elements.
  * Note this is potentially expensive if there are a lot of EFTs in use.
  * @return  True on success, false on failure. */
bool FE_mesh::removeElementNodesFromGroup(DsLabelIndex elementIndex, DsLabelsGroup& nodeLabelsGroup)
{
	if (elementIndex < 0)
		return false;
	for (int i = 0; i < this->elementFieldTemplateDataCount; ++i)
	{
		const FE_mesh_element_field_template_data *eftData = this->elementFieldTemplateData[i];
		if (!eftData)
			continue;
		const int localNodeCount = eftData->getElementfieldtemplate()->getNumberOfLocalNodes();
		if (localNodeCount)
		{
			const DsLabelIndex *nodeIndexes = eftData->getElementNodeIndexes(elementIndex);
			if (nodeIndexes)
				for (int n = 0; n < localNodeCount; ++n)
					if (nodeIndexes[n] >= 0)
					{
						const int result = nodeLabelsGroup.setIndex(nodeIndexes[n], false);
						if ((result != CMZN_OK) && (result != CMZN_ERROR_NOT_FOUND))
							return false;
					}
		}
	}
	return true;
}

bool FE_mesh::isElementAncestor(DsLabelIndex elementIndex,
	const FE_mesh *descendantMesh, DsLabelIndex descendantIndex)
{
	if ((!descendantMesh) || (descendantIndex < 0))
		return false;
	if (this == descendantMesh)
		return (elementIndex == descendantIndex);
	if (!descendantMesh->parentMesh)
		return false;
	const DsLabelIndex *parents;
	const int parentsCount = descendantMesh->getElementParents(descendantIndex, parents);
	if (0 == parentsCount)
		return false;
	if (descendantMesh->parentMesh == this)
	{
		for (int p = 0; p < parentsCount; ++p)
			if (parents[p] == elementIndex)
				return true;
	}
	else if (descendantMesh->parentMesh->parentMesh == this)
	{
		for (int p = 0; p < parentsCount; ++p)
		{
			const DsLabelIndex *parentsParents;
			const int parentsParentsCount = descendantMesh->parentMesh->getElementParents(parents[p], parentsParents);
			for (int pp = 0; pp < parentsParentsCount; ++pp)
				if (parentsParents[pp] == elementIndex)
					return true;
		}
	}
	return false;
}

bool FE_mesh::isElementExterior(DsLabelIndex elementIndex)
{
	if (!this->parentMesh)
		return false;
	const DsLabelIndex *parents;
	const int parentsCount = this->getElementParents(elementIndex, parents);
	if (0 == parentsCount)
		return false;
	const DsLabelIndex *parentsParents;
	if (1 == parentsCount)
	{
		if ((!this->parentMesh->parentMesh) || (0 == this->parentMesh->getElementParents(parents[0], parentsParents)))
			return true;
	}
	else
	{
		for (int i = 0; i < parentsCount; ++i)
			if (1 == this->parentMesh->getElementParents(parents[i], parentsParents))
				return true;
	}
	return false;
}

DsLabelIndex FE_mesh::getElementParentOnFace(DsLabelIndex elementIndex, cmzn_element_face_type faceType)
{
	if (!this->parentMesh)
		return DS_LABEL_INDEX_INVALID;
	const DsLabelIndex *parents;
	const int parentsCount = this->getElementParents(elementIndex, parents);
	if (0 == parentsCount)
		return DS_LABEL_INDEX_INVALID;
	if ((CMZN_ELEMENT_FACE_TYPE_ANY_FACE == faceType) || (CMZN_ELEMENT_FACE_TYPE_ALL == faceType))
		return parents[0];
	FE_mesh *parentParentMesh = this->parentMesh->parentMesh;
	for (int i = 0; i < parentsCount; ++i)
	{
		const DsLabelIndex *parentsParents;
		int parentsParentsCount;
		if ((parentParentMesh) && (parentsParentsCount = this->parentMesh->getElementParents(parents[i], parentsParents)))
		{
			for (int j = 0; j < parentsParentsCount; ++j)
				if (parentParentMesh->isElementFaceOfType(parentsParents[j], parents[i], faceType))
					return parents[i];
		}
		else
		{
			if (parentMesh->isElementFaceOfType(parents[i], elementIndex, faceType))
				return parents[i];
		}
	}
	return DS_LABEL_INDEX_INVALID;
}

/** return the index of neighbour element on faceNumber, if any. Looks to first parent first
 * Copes with element wrapping around and joining itself; will find the other face.
 * @param newFaceNumber  If neighbour found, this gives the face it is on.
 */
DsLabelIndex FE_mesh::getElementFirstNeighbour(DsLabelIndex elementIndex, int faceNumber, int &newFaceNumber)
{
	ElementShapeFaces *elementShapeFaces;
	const DsLabelIndex *faces;
	DsLabelIndex faceIndex;
	if ((this->faceMesh) && (elementShapeFaces = this->getElementShapeFaces(elementIndex)) &&
		(faces = elementShapeFaces->getElementFaces(elementIndex)) &&
		(0 < faceNumber) && (faceNumber < elementShapeFaces->getFaceCount()) &&
		(0 <= (faceIndex = faces[faceNumber])))
	{
		const DsLabelIndex *parents;
		const int parentsCount = this->faceMesh->getElementParents(faceIndex, parents);
		for (int i = 0; i < parentsCount; ++i)
		{
			if (parents[i] != elementIndex)
			{
				newFaceNumber = this->getElementFaceNumber(parents[i], faceIndex);
				return parents[i];
			}
		}
		if (parentsCount > 1)
		{
			// faceIndex is on more than one face of elementIndex; change to other face number
			for (int i = elementShapeFaces->getFaceCount() - 1; i >= 0; --i)
				if ((i != faceNumber) && (faces[i] == faceIndex))
				{
					newFaceNumber = i;
					return elementIndex;
				}
		}
	}
	return DS_LABEL_INDEX_INVALID;
}

/**
 * Find or create an element in this mesh that can be used on face number of
 * the parent element. The face is added to the parent.
 * The new face element is added to this mesh, but without adding faces.
 * Must be between calls to begin_define_faces/end_define_faces.
 * Can only match faces correctly for coordinate fields with standard node
 * to element maps and no versions.
 * The element type node sequence list is updated with any new face.
 *
 * @param parentIndex  Index of parent element in parentMesh, to find or create
 * face for.
 * @param faceNumber  Face number on parent, starting at 0.
 * @param faceIndex  On successful return, set to new faceIndex or
 * DS_LABEL_INDEX_INVALID if no face needed (for collapsed element face).
 * @return  Index of new face or DS_LABEL_INDEX_INVALID if failed.
 * @return  Result OK on success, ERROR_NOT_FOUND if no nodes available
 * for defining faces, otherwise any other error.
 */
int FE_mesh::findOrCreateFace(DsLabelIndex parentIndex, int faceNumber, DsLabelIndex& faceIndex)
{
	faceIndex = DS_LABEL_INDEX_INVALID;
	cmzn_element *parentElement = this->parentMesh->getElement(parentIndex);
	int return_code = CMZN_OK;
	FE_element_type_node_sequence *element_type_node_sequence =
		CREATE(FE_element_type_node_sequence)(return_code, parentElement, faceNumber);
	if (!element_type_node_sequence)
	{
		return return_code;
	}

	ACCESS(FE_element_type_node_sequence)(element_type_node_sequence);
	if (!FE_element_type_node_sequence_is_collapsed(element_type_node_sequence))
	{
		FE_element_type_node_sequence *existing_element_type_node_sequence =
			FE_element_type_node_sequence_list_find_match(
			this->element_type_node_sequence_list, element_type_node_sequence);
		if (existing_element_type_node_sequence)
		{
			cmzn_element *face = FE_element_type_node_sequence_get_FE_element(existing_element_type_node_sequence);
			if (!face)
				return_code = CMZN_ERROR_GENERAL;
			else
			{
				faceIndex = face->getIndex();
				return_code = this->parentMesh->setElementFace(parentIndex, faceNumber, faceIndex);
			}
		}
		else
		{
			FE_element_shape *parentShape = this->parentMesh->getElementShape(parentIndex);
			FE_element_shape *faceShape = get_FE_element_shape_of_face(parentShape, faceNumber, this->fe_region);
			if (!faceShape)
				return_code = CMZN_ERROR_GENERAL;
			else
			{
				cmzn_element *face = this->get_or_create_FE_element_with_identifier(/*identifier*/-1, faceShape);
				if (!face)
					return_code = CMZN_ERROR_GENERAL;
				else
				{
					FE_element_type_node_sequence_set_FE_element(element_type_node_sequence, face);
					faceIndex = face->getIndex();
					return_code = this->parentMesh->setElementFace(parentIndex, faceNumber, faceIndex);
					if (CMZN_OK == return_code)
					{
						if (!ADD_OBJECT_TO_LIST(FE_element_type_node_sequence)(
								element_type_node_sequence, this->element_type_node_sequence_list))
							return_code = CMZN_ERROR_GENERAL;
					}
					cmzn_element::deaccess(face);
				}
			}
		}
	}
	DEACCESS(FE_element_type_node_sequence)(&element_type_node_sequence);
	return return_code;
}

/**
 * Recursively define faces for element, creating and adding them to face
 * mesh if they don't already exist.
 * Always call between FE_region_begin/end_define_faces.
 * Records but doesn't notify of element/face/field changes.
 * Always call between FE_region_begin/end_changes.
 * Function ensures that elements share existing faces and lines in preference to
 * creating new ones if they have matching dimension and nodes.
 * @return  CMZN_OK on success, otherwise any error code.
 */
int FE_mesh::defineElementFaces(DsLabelIndex elementIndex)
{
	if (!(this->faceMesh && this->definingFaces && (elementIndex >= 0)))
		return CMZN_ERROR_ARGUMENT;
	ElementShapeFaces *elementShapeFaces = this->getElementShapeFaces(elementIndex);
	if (!elementShapeFaces)
	{
		display_message(ERROR_MESSAGE, "FE_mesh::defineElementFaces.  Missing ElementShapeFaces");
		return CMZN_ERROR_ARGUMENT;
	}
	const int faceCount = elementShapeFaces->getFaceCount();
	if (0 == faceCount)
		return CMZN_OK;
	DsLabelIndex *faces = elementShapeFaces->getOrCreateElementFaces(elementIndex);
	if (!faces)
		return CMZN_ERROR_GENERAL;
	int return_code = CMZN_OK;
	int newFaceCount = 0;
	for (int faceNumber = 0; faceNumber < faceCount; ++faceNumber)
	{
		DsLabelIndex faceIndex = faces[faceNumber];
		if (faceIndex < 0)
		{
			return_code = this->faceMesh->findOrCreateFace(elementIndex, faceNumber, faceIndex);
			if (CMZN_OK != return_code)
			{
				if (CMZN_ERROR_NOT_FOUND == return_code)
				{
					continue;
				}
				break;
			}
			if (faceIndex >= 0)
				++newFaceCount;
		}
		if ((this->dimension > 2) && (DS_LABEL_INDEX_INVALID != faceIndex))
		{
			// recursively add faces of faces, whether existing or new
			return_code = this->faceMesh->defineElementFaces(faceIndex);
			if (CMZN_OK != return_code)
			{
				break;
			}
		}
	}
	if (newFaceCount)
	{
		this->changeLog->setIndexChange(elementIndex, DS_LABEL_CHANGE_TYPE_DEFINITION);
		if (fe_region)
		{
			this->fe_region->FE_field_all_change(CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field));
			fe_region->update();
		}
	}
	if ((CMZN_OK != return_code) && (CMZN_ERROR_NOT_FOUND != return_code))
	{
		display_message(ERROR_MESSAGE, "FE_mesh::defineElementFaces.  Failed");
	}
	return (return_code);
}

/**
 * Creates a list of FE_element_type_node_sequence, and
 * if mesh dimension < MAXIMUM_ELEMENT_XI_DIMENSIONS fills it with sequences
 * for this element. Fails if any two faces have the same shape and nodes.
 */
int FE_mesh::begin_define_faces()
{
	if (this->element_type_node_sequence_list)
	{
		display_message(ERROR_MESSAGE, "FE_mesh::begin_define_faces.  Already defining faces");
		return CMZN_ERROR_ALREADY_EXISTS;
	}
	this->element_type_node_sequence_list = CREATE(LIST(FE_element_type_node_sequence))();
	if (!this->element_type_node_sequence_list)
	{
		display_message(ERROR_MESSAGE, "FE_mesh::begin_define_faces.  Could not create node sequence list");
		return CMZN_ERROR_MEMORY;
	}
	this->definingFaces = true;
	int return_code = CMZN_OK;
	if (this->dimension < MAXIMUM_ELEMENT_XI_DIMENSIONS)
	{
		cmzn_elementiterator_id iter = this->createElementiterator();
		cmzn_element_id element = 0;
		FE_element_type_node_sequence *element_type_node_sequence;
		while (0 != (element = cmzn_elementiterator_next_non_access(iter)))
		{
			element_type_node_sequence = CREATE(FE_element_type_node_sequence)(return_code, element);
			if (!element_type_node_sequence)
			{
				if (CMZN_ERROR_NOT_FOUND == return_code)
				{
					return_code = CMZN_OK;
					continue;
				}
				display_message(ERROR_MESSAGE, "FE_mesh::begin_define_faces.  "
					"Could not create FE_element_type_node_sequence for %d-D element %d",
					this->dimension, get_FE_element_identifier(element));
				break;
			}
			if (!ADD_OBJECT_TO_LIST(FE_element_type_node_sequence)(
				element_type_node_sequence, this->element_type_node_sequence_list))
			{
				display_message(WARNING_MESSAGE, "FE_mesh::begin_define_faces.  "
					"Could not add FE_element_type_node_sequence for %d-D element %d.",
					this->dimension, get_FE_element_identifier(element));
				FE_element_type_node_sequence *existing_element_type_node_sequence =
					FE_element_type_node_sequence_list_find_match(
						this->element_type_node_sequence_list, element_type_node_sequence);
				if (existing_element_type_node_sequence)
				{
					display_message(WARNING_MESSAGE,
						"Reason: Existing %d-D element %d uses same node list, and will be used for face matching.",
						this->dimension, get_FE_element_identifier(
							FE_element_type_node_sequence_get_FE_element(existing_element_type_node_sequence)));
				}
				DESTROY(FE_element_type_node_sequence)(&element_type_node_sequence);
			}
		}
		cmzn_elementiterator_destroy(&iter);
	}
	return return_code;
}

void FE_mesh::end_define_faces()
{
	if (this->element_type_node_sequence_list)
		DESTROY(LIST(FE_element_type_node_sequence))(&this->element_type_node_sequence_list);
	else
		display_message(ERROR_MESSAGE, "FE_mesh::end_define_faces.  Wasn't defining faces");
	this->definingFaces = false;
}

/**
 * Ensures faces of elements in mesh exist in face mesh.
 * Recursively does same for faces in face mesh.
 * Call between begin/end_define_faces and begin/end_change.
 */
int FE_mesh::define_faces()
{
	DsLabelIterator *iter = this->labels.createLabelIterator();
	if (!iter)
		return CMZN_ERROR_GENERAL;
	int return_code = CMZN_OK;
	DsLabelIndex elementIndex;
	int successCount = 0;
	while ((elementIndex = iter->nextIndex()) != DS_LABEL_INDEX_INVALID)
	{
		const int result = this->defineElementFaces(elementIndex);
		if (result != CMZN_OK)
		{
			return_code = result;
			if (result == CMZN_ERROR_NOT_FOUND)
			{
				continue;
			}
			break;
		}
		++successCount;
	}
	cmzn::Deaccess(iter);
	if ((return_code == CMZN_ERROR_NOT_FOUND) && successCount)
	{
		return_code = CMZN_WARNING_PART_DONE;
	}
	return return_code;
}

namespace {

struct FE_mesh_and_element_index
{
	FE_mesh *mesh;
	const DsLabelIndex elementIndex;
};

int FE_field_clear_element_varying_data(FE_field *field, void *mesh_and_element_index_void)
{
	auto args = static_cast<FE_mesh_and_element_index*>(mesh_and_element_index_void);
	FE_mesh *mesh = args->mesh;
	const DsLabelIndex elementIndex = args->elementIndex;
	FE_mesh_field_data *meshFieldData = field->getMeshFieldData(mesh);
	if (meshFieldData)
		meshFieldData->clearElementData(elementIndex);
	if ((field->getValueType() == ELEMENT_XI_VALUE) && (field->getElementXiHostMesh() == args->mesh))
	{
		// clear embedding of nodes & data points in this element
		for (int i = 0; i < 2; ++i)
		{
			FE_nodeset *nodeset = mesh->get_FE_region()->nodesets[i];
			FE_mesh_embedded_node_field *embeddedNodeField = field->getEmbeddedNodeField(nodeset);
			if (embeddedNodeField)
			{
				DsLabelIndex nodeIndex;
				while ((nodeIndex = embeddedNodeField->getLastNodeIndex(elementIndex)) >= 0)
				{
					// this removes the node index from the embeddedNodeField, so loop should end
					set_FE_nodal_element_xi_value(nodeset->getNode(nodeIndex), field, /*component*/0, /*element*/nullptr, /*xi*/nullptr);
				}
			}
		}
	}
	return 1;
}

}

/**
 * Removes element and all its faces that are not shared with other elements
 * from mesh. Releases per-element data including nodes.
 * Records remove element change but doesn't notify.
 * Note: expects FE_region change cache to be on or caller to call FE_region update.
 * Note: expects caller to record field changes.
 * This function is recursive.
 * @param elementIndex  A valid element index. Not checked!
 */
int FE_mesh::removeElementPrivate(DsLabelIndex elementIndex)
{
	int return_code = 1;
	cmzn_element *element = this->fe_elements.getValue(elementIndex);
	if (element)
	{
		// clear data for this element stored with field, e.g. per-element DOFs
		FE_mesh_and_element_index mesh_and_element_index = { this, elementIndex };
		FE_region_for_each_FE_field(this->fe_region, FE_field_clear_element_varying_data, &mesh_and_element_index);
		// clear element field templates for element in all mesh field templates
		// note this clears nodes and scale factors for element once last usage of EFT for element is removed
		for (auto mftIter = this->meshFieldTemplates.begin(); mftIter != this->meshFieldTemplates.end(); ++mftIter)
		{
			FE_mesh_field_template *mft = *mftIter;
			mft->setElementfieldtemplateIndex(elementIndex, FE_mesh_field_template::ELEMENT_FIELD_TEMPLATE_DATA_INDEX_INVALID);
		}
		element->invalidate();
		this->fe_elements.setValue(elementIndex, 0);
		cmzn_element::deaccess(element);
		this->changeLog->setIndexChange(elementIndex, DS_LABEL_CHANGE_TYPE_REMOVE);
		if (this->parentMesh)
			this->clearElementParents(elementIndex);
		if (this->faceMesh)
			this->clearElementFaces(elementIndex);
		this->labels.removeLabel(elementIndex);
		if (0 == this->labels.getSize())
			this->clear();
	}
	else
	{
		display_message(ERROR_MESSAGE, "FE_mesh::removeElementPrivate.  No element object at index");
		return_code = 0;
	}
	return (return_code);
}

/**
 * Removes <element> and all its faces that are not shared with other elements
 * from <fe_region>.
 * FE_region_begin/end_change are called internally to reduce change messages to
 * one per call. User should place calls to the begin/end_change functions
 * around multiple calls to this function.
 * This function is recursive.
 */
int FE_mesh::destroyElement(cmzn_element *element)
{
	if ((!element) || (element->getIndex() < 0))
		return CMZN_ERROR_ARGUMENT;
	FE_region_begin_change(this->fe_region);
	int return_code = this->removeElementPrivate(element->getIndex());
	if ((CMZN_OK == return_code) && (this->fe_region))
		this->fe_region->FE_field_all_change(CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field));
	FE_region_end_change(this->fe_region);
	return return_code;
}

/**
 * Destroy all the elements in FE_mesh, and all their faces
 * that are not shared with other elements from <fe_region>.
 * Caches changes to ensure only one change message per call.
 */
int FE_mesh::destroyAllElements()
{
	int return_code = CMZN_OK;
	FE_region_begin_change(fe_region);
	// can't use an iterator as invalidated when element removed
	const int oldSize = this->getSize();
	const DsLabelIndex indexLimit = this->labels.getIndexSize();
	const bool contiguous = this->labels.isContiguous();
	for (DsLabelIndex elementIndex = 0; elementIndex < indexLimit; ++elementIndex)
	{
		// must handle holes left in identifier array by deleted elements
		if (contiguous || (0 <= this->getElementIdentifier(elementIndex)))
		{
			return_code = this->removeElementPrivate(elementIndex);
			if (return_code != CMZN_OK)
				break;
		}
	}
	// mark all fields changed if any removed
	if ((this->getSize() != oldSize) && (this->fe_region))
		this->fe_region->FE_field_all_change(CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field));
	FE_region_end_change(fe_region);
	return (return_code);
}

/**
 * Destroy all the elements in labelsGroup, and all their faces
 * that are not shared with other elements from <fe_region>.
 * Caches changes to ensure only one change message per call.
 */
int FE_mesh::destroyElementsInGroup(DsLabelsGroup& labelsGroup)
{
	int return_code = CMZN_OK;
	FE_region_begin_change(this->fe_region);
	const int oldSize = this->getSize();
	// can't use an iterator as invalidated when element removed
	DsLabelIndex elementIndex = -1; // DS_LABEL_INDEX_INVALID
	while (labelsGroup.incrementIndex(elementIndex))
	{
		return_code = this->removeElementPrivate(elementIndex);
		if (return_code != CMZN_OK)
			break;
	}
	// mark all fields changed if any removed
	if ((this->getSize() != oldSize) && (this->fe_region))
		this->fe_region->FE_field_all_change(CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field));
	FE_region_end_change(this->fe_region);
	return (return_code);
}

FieldDerivative *FE_mesh::getHigherFieldDerivative(const FieldDerivative& fieldDerivative)
{
	if ((fieldDerivative.getMesh()) && (fieldDerivative.getMesh() != this))
	{
		display_message(ERROR_MESSAGE, "FE_mesh::getHigherFieldDerivative.  Cannot create derivative w.r.t. multiple meshes");
		return nullptr;
	}
	if (fieldDerivative.getFieldparameters())
		return fieldDerivative.getFieldparameters()->getFieldDerivativeMixed(this,
			fieldDerivative.getMeshOrder() + 1, fieldDerivative.getParameterOrder());
	return this->getFieldDerivative(fieldDerivative.getMeshOrder() + 1);
}

DsLabelIndex FE_mesh::createScaleFactorIndex()
{
	if (this->scaleFactors.setValue(this->scaleFactorsIndexSize, 0.0) &&
		this->scaleFactorUsageCounts.setValue(this->scaleFactorsIndexSize, 1))
	{
		++this->scaleFactorsCount;
		++this->scaleFactorsIndexSize;
		return (this->scaleFactorsIndexSize - 1);
	}
	display_message(ERROR_MESSAGE, "FE_mesh::createScaleFactorIndex.  Failed");
	return DS_LABEL_INDEX_INVALID;
}

DsLabelIndex FE_mesh::getOrCreateScaleFactorIndex(cmzn_elementfieldtemplate_scale_factor_type scaleFactorType,
	DsLabelIndex objectIndex, int scaleFactorIdentifier)
{
	if (isScaleFactorTypeElement(scaleFactorType))
	{
		if (scaleFactorIdentifier != 0)
		{
			display_message(ERROR_MESSAGE, "FE_mesh::getOrCreateScaleFactorIndex.  Element type scale factor identifier must be 0");
			return DS_LABEL_INDEX_INVALID;
		}
	}
	else if (scaleFactorIdentifier < 1)
	{
		display_message(ERROR_MESSAGE, "FE_mesh::getOrCreateScaleFactorIndex.  Invalid scale factor identifier");
		return DS_LABEL_INDEX_INVALID;
	}
	if ((!isScaleFactorTypeGlobal(scaleFactorType)) && (objectIndex < 0))
	{
		display_message(ERROR_MESSAGE, "FE_mesh::getOrCreateScaleFactorIndex.  Invalid %s index",
			isScaleFactorTypeElement(scaleFactorType) ? "element" : "node");
		return DS_LABEL_INDEX_INVALID;
	}
	switch (scaleFactorType)
	{
	case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_GENERAL:
	case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_PATCH:
	{
		// no map: stored in FE_mesh_element_field_template_data for element
		return this->createScaleFactorIndex(); // returns DS_LABEL_INDEX_INVALID if failed
	} break;
	case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_GENERAL:
	case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_PATCH:
	{
		const DsLabelIndex packedIdentifier = (scaleFactorType == CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_GENERAL) ?
			scaleFactorIdentifier : -scaleFactorIdentifier;
		std::map<int, DsLabelIndex>::iterator globalIter = this->globalScaleFactorsIndex.find(packedIdentifier);
		if (globalIter != this->globalScaleFactorsIndex.end())
		{
			const DsLabelIndex scaleFactorIndex = globalIter->second;
			// increment usage count
			int *usageCountAddress = this->scaleFactorUsageCounts.getAddress(scaleFactorIndex);
			++(*usageCountAddress);
			return scaleFactorIndex;
		}
		const DsLabelIndex scaleFactorIndex = this->createScaleFactorIndex();
		if (scaleFactorIndex >= 0)
		{
			// add to global map
			this->globalScaleFactorsIndex[packedIdentifier] = scaleFactorIndex;
		}
		return scaleFactorIndex;
	} break;
	case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_GENERAL:
	case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_PATCH:
	{
		const DsLabelIndex packedIdentifier = (scaleFactorType == CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_GENERAL) ?
			scaleFactorIdentifier : -scaleFactorIdentifier;
		DsLabelIndex **identifierIndexesAddress = this->nodeScaleFactorsIndex.getOrCreateAddress(objectIndex);
		if (!identifierIndexesAddress)
		{
			display_message(ERROR_MESSAGE, "FE_mesh::getOrCreateScaleFactorIndex.  Failed to get node scale factor map address");
			return DS_LABEL_INDEX_INVALID;
		}
		int nodeScaleFactorCount = 0;
		DsLabelIndex *identifierIndexes = *identifierIndexesAddress;
		if (identifierIndexes)
		{
			DsLabelIndex *identifier = identifierIndexes;
			while (*identifier) // since zero-terminated array of identifier, index pairs
			{
				if (packedIdentifier == *identifier)
				{
					const DsLabelIndex scaleFactorIndex = identifier[1];
					// increment usage count
					int *usageCountAddress = this->scaleFactorUsageCounts.getAddress(scaleFactorIndex);
					++(*usageCountAddress);
					return scaleFactorIndex;
				}
				identifier += 2;
				++nodeScaleFactorCount;
			}
		}
		// enlarge identifier index map for new scale factor index
		DsLabelIndex *newIdentifierIndexes = new DsLabelIndex[nodeScaleFactorCount*2 + 3];
		if (!newIdentifierIndexes)
		{
			display_message(ERROR_MESSAGE, "FE_mesh::getOrCreateScaleFactorIndex.  Failed to resize node scale factor map");
			return DS_LABEL_INDEX_INVALID;
		}
		const DsLabelIndex scaleFactorIndex = this->createScaleFactorIndex();
		if (scaleFactorIndex < 0)
		{
			delete[] newIdentifierIndexes;
			return DS_LABEL_INDEX_INVALID;
		}
		if (identifierIndexes)
		{
			memcpy(newIdentifierIndexes, identifierIndexes, sizeof(DsLabelIndex)*nodeScaleFactorCount*2);
			delete[] identifierIndexes;
			identifierIndexes = 0;
		}
		newIdentifierIndexes[nodeScaleFactorCount*2] = packedIdentifier;
		newIdentifierIndexes[nodeScaleFactorCount*2 + 1] = scaleFactorIndex;
		newIdentifierIndexes[nodeScaleFactorCount*2 + 2] = 0;  // zero terminated array
		*identifierIndexesAddress = newIdentifierIndexes;
		return scaleFactorIndex;
	} break;
	default:
		display_message(ERROR_MESSAGE, "FE_mesh::getOrCreateScaleFactorIndex.  Invalid scale factor type");
		break;
	}
	return DS_LABEL_INDEX_INVALID;
}

void FE_mesh::releaseScaleFactorIndex(DsLabelIndex scaleFactorIndex,
	cmzn_elementfieldtemplate_scale_factor_type scaleFactorType,
	DsLabelIndex objectIndex, int scaleFactorIdentifier)
{
	if (scaleFactorIndex < 0)
	{
		display_message(ERROR_MESSAGE, "FE_mesh::releaseScaleFactorIndex.  Invalid scale factor index");
		return;
	}
	int *usageCountAddress = this->scaleFactorUsageCounts.getAddress(scaleFactorIndex);
	if (!usageCountAddress)
	{
		display_message(ERROR_MESSAGE, "FE_mesh::releaseScaleFactorIndex.  Invalid scale factor index; could not get usage count "
			"for scale factor index %d, type %d, object index %d, identifier %d",
			scaleFactorIndex, scaleFactorType, objectIndex, scaleFactorIdentifier);
		return;
	}
	--(*usageCountAddress);
	if (*usageCountAddress <= 0)
	{
		if (*usageCountAddress < 0)
		{
			display_message(ERROR_MESSAGE, "FE_mesh::releaseScaleFactorIndex.  Negative scale factor usage count "
				"for scale factor index %d, type %d, object index %d, identifier %d",
				scaleFactorIndex, scaleFactorType, objectIndex, scaleFactorIdentifier);
			return;
		}
		--this->scaleFactorsCount;
		switch (scaleFactorType)
		{
		case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_GENERAL:
		case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_PATCH:
		{
			// nothing to do
		} break;
		case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_GENERAL:
		case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_PATCH:
		{
			const DsLabelIndex packedIdentifier = (scaleFactorType == CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_GENERAL) ?
				scaleFactorIdentifier : -scaleFactorIdentifier;
			std::map<int, DsLabelIndex>::iterator globalIter = this->globalScaleFactorsIndex.find(packedIdentifier);
			if (globalIter != this->globalScaleFactorsIndex.end())
			{
				this->globalScaleFactorsIndex.erase(globalIter);
			}
			else
			{
				display_message(ERROR_MESSAGE, "FE_mesh::releaseScaleFactorIndex.  Index missing from global scale factor map "
					"for scale factor index %d, type %d, identifier %d",
					scaleFactorIndex, scaleFactorType, scaleFactorIdentifier);
			}
		} break;
		case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_GENERAL:
		case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_PATCH:
		{
			const DsLabelIndex packedIdentifier = (scaleFactorType == CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_GENERAL) ?
				scaleFactorIdentifier : -scaleFactorIdentifier;
			DsLabelIndex **identifierIndexesAddress = this->nodeScaleFactorsIndex.getOrCreateAddress(objectIndex);
			if (!identifierIndexesAddress)
			{
				display_message(ERROR_MESSAGE, "FE_mesh::releaseScaleFactorIndex.  Failed to get node scale factor map address");
				return;
			}
			DsLabelIndex *identifierIndexes = *identifierIndexesAddress;
			if (!identifierIndexes)
			{
				display_message(ERROR_MESSAGE, "FE_mesh::releaseScaleFactorIndex.  Failed to get node scale factor map");
				return;
			}
			DsLabelIndex *identifier = identifierIndexes;
			while (*identifier)
			{
				if (packedIdentifier == *identifier)
				{
					while (identifier[2])
					{
						identifier[0] = identifier[2];
						identifier[1] = identifier[3];
						identifier += 2;
					}
					*identifier = 0;
					if (identifier == identifierIndexes)
					{
						delete[] identifierIndexes;
						*identifierIndexesAddress = 0;
					}
					return;
				}
				identifier += 2;
			}
			display_message(ERROR_MESSAGE, "FE_mesh::releaseScaleFactorIndex.  Missing mapping from "
				"scale factor type %d, node index %d, identifier %d to index %d",
				scaleFactorType, objectIndex, scaleFactorIdentifier, scaleFactorIndex);
		} break;
		default:
			display_message(ERROR_MESSAGE, "FE_mesh::releaseScaleFactorIndex.  Invalid scale factor type");
			break;
		}
	}
}

namespace {

int FE_field_add_real_type_to_vector(struct FE_field *field, void *field_vector_void)
{
	if (get_FE_field_value_type(field) == FE_VALUE_VALUE)
	{
		std::vector<FE_field *> *field_vector = static_cast<std::vector<FE_field *> *>(field_vector_void);
		field_vector->push_back(field);
	}
	return 1;
}

}

/**
 * Attempts to convert all node parameter maps in mesh from using legacy DOF
 * indexes to use derivative labels and versions. Requires nodes to already
 * exist and hold information about labels and versions for all fields.
 * Must call prior to FE_mesh canMerge/merge.
 * Note the current implementation requires each legacy parameter map to
 * convert to a single new parameter map, but due to the legacy map's use of
 * indexes it is possible it means different things depending on the node
 * field components for different fields.
 * NOTE: this is relatively expensive.
 * @param targetNodeset  Optional global/target nodeset containing existing
 * node field definitions if not found in nodeset attached to this mesh.
 * @return  True on success, false if any legacy node parameter maps could
 * not be converted.
 */
bool FE_mesh::checkConvertLegacyNodeParameters(FE_nodeset *targetNodeset)
{
	if (!this->nodeset)
	{
		display_message(ERROR_MESSAGE, "FE_mesh::checkConvertLegacyNodeParameters.  Mesh does not have a nodeset");
		return false;
	}
	std::vector<FE_field*> fields;
	if (!FE_region_for_each_FE_field(this->fe_region, FE_field_add_real_type_to_vector, (void *)&fields))
		return false;
	const int fieldCount = static_cast<int>(fields.size());

	for (int i = 0; i < this->elementFieldTemplateDataCount; ++i)
	{
		const FE_mesh_element_field_template_data *eftData = this->elementFieldTemplateData[i];
		if (!eftData)
			continue;
		FE_element_field_template *eft = eftData->getElementfieldtemplate();
		if (!eft->hasNodeParameterLegacyIndex())
			continue;
		const DsLabelIndex elementIndexLimit = eftData->getElementIndexLimit();
		const DsLabelIndex elementIndexStart = eftData->getElementIndexStart();

		// store the legacy DOF indexes for checking once overwritten by value type/version
		std::vector<int> legacyDOFIndexes(eft->getTotalTermCount());
		const int functionCount = eft->getNumberOfFunctions();
		int tt = 0;
		for (int fn = 0; fn < functionCount; ++fn)
		{
			const int termCount = eft->getFunctionNumberOfTerms(fn);
			for (int t = 0; t < termCount; ++t)
			{
				legacyDOFIndexes[tt] = eft->getTermNodeLegacyIndex(fn, t);
				++tt;
			}
		}
		// working space for storing node field components
		const int localNodeCount = eft->getNumberOfLocalNodes();
		std::vector<const FE_node_field_template *> nodeFieldTemplates(localNodeCount);

		bool first = true;
		for (int f = 0; f < fieldCount; ++f)
		{
			FE_field *field = fields[f];
			const char *fieldName = get_FE_field_name(field);
			FE_field *targetField = (targetNodeset) ? FE_region_get_FE_field_from_name(targetNodeset->get_FE_region(), fieldName) : 0; // this may not exist
			FE_mesh_field_data *meshFieldData = field->getMeshFieldData(this);
			if (!meshFieldData)
				continue;
			const int componentCount = get_FE_field_number_of_components(field);
			for (DsLabelIndex elementIndex = elementIndexStart; elementIndex < elementIndexLimit; ++elementIndex)
			{
				const DsLabelIndex *nodeIndexes = 0;
				for (int c = 0; c < componentCount; ++c)
				{
					FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
					if (!mft->hasElementfieldtemplate(elementIndex, eft))
						continue;
					// must check all node field components since may have different derivatives and versions
					if (!nodeIndexes)
					{
						nodeIndexes = eftData->getElementNodeIndexes(elementIndex);
						if (!nodeIndexes)
						{
							display_message(ERROR_MESSAGE, "Field %s component %d element %d has no local nodes set",
								fieldName, c + 1, this->getElementIdentifier(elementIndex));
							return false;
						}
					}
					for (int n = 0; n < localNodeCount; ++n)
					{
						cmzn_node *node = nodeset->getNode(nodeIndexes[n]);
						if (!node)
						{
							display_message(ERROR_MESSAGE, "Field %s component %d element %d local node %d is not set",
								fieldName, c + 1, this->getElementIdentifier(elementIndex), n + 1);
							return false;
						}
						const FE_node_field *nodeField = node->getNodeField(field);
						if (!nodeField)
						{
							DsLabelIdentifier nodeIdentifier = nodeset->getNodeIdentifier(nodeIndexes[n]);
							cmzn_node *targetNode = (targetNodeset) ? targetNodeset->findNodeByIdentifier(nodeIdentifier) : 0;
							if ((targetNode) && (targetField))
							{
								nodeField = targetNode->getNodeField(targetField);
							}
							if (!nodeField)
							{
								display_message(ERROR_MESSAGE, "No parameters defined for field %s at element %d local node %d (global node %d). "
									"PROBABLE FIX: Read nodes before elements with legacy element parameter maps.",
									fieldName, this->getElementIdentifier(elementIndex), n + 1, nodeIdentifier);
								return false;
							}
						}
						nodeFieldTemplates[n] = nodeField->getComponent(c);
						if (!nodeFieldTemplates[n])
						{
							display_message(ERROR_MESSAGE, "No parameters for field %s component %d at element %d local node %d (global node %d)."
								"PROBABLE FIX: Read correct nodes before elements with legacy element parameter maps.",
								fieldName, c + 1, this->getElementIdentifier(elementIndex), n + 1, nodeset->getNodeIdentifier(nodeIndexes[n]));
							return false;
						}
					}
					if (first)
					{
						if (!eft->convertNodeParameterLegacyIndexes(legacyDOFIndexes, nodeFieldTemplates))
						{
							display_message(ERROR_MESSAGE, "Field %s component %d element %d.  Failed to convert legacy DOF indexes."
								"POSSIBLE FIX: Read correct nodes before elements with legacy element parameter maps.",
								fieldName, c + 1, this->getElementIdentifier(elementIndex));
							return false;
						}
						first = false;
					}
					else if (!eft->checkNodeParameterLegacyIndexes(legacyDOFIndexes, nodeFieldTemplates))
					{
						display_message(ERROR_MESSAGE, "Field %s component %d element %d.  Failed to convert legacy DOF indexes, or these mapped to different node value labels or versions."
							"THIS CASE IS NOT IMPLEMENTED! Talk to developers, or manually change problem elements to use new value labels style in input file.",
							fieldName, c + 1, this->getElementIdentifier(elementIndex));
						return false;
					}
				}
			}
		}
	}
	return true;
}

/**
 * Check that the source mesh can be merged into this mesh. Currently only
 * checks that element shape and faces are not changing.
 * Assumes FE_mesh::checkConvertLegacyNodeParameters() has been called and returned true.
 * @param source  Source mesh to check for merge.
 * @return  True if source mesh can be merged, otherwise false. If true, can
 * call FE_mesh::merge().
 */
bool FE_mesh::canMerge(const FE_mesh &source) const
{
	if (source.dimension != this->dimension)
	{
		display_message(ERROR_MESSAGE, "FE_mesh::canMerge.  Source mesh has wrong dimension");
		return false;
	}
	DsLabelIterator *iter = source.labels.createLabelIterator();
	if (!iter)
		return false;
	bool result = true;
	DsLabelIndex sourceIndex;
	while ((sourceIndex = iter->nextIndex()) >= 0)
	{
		const DsLabelIdentifier identifier = source.getElementIdentifier(sourceIndex);
		const DsLabelIndex targetIndex = this->labels.findLabelByIdentifier(identifier);
		const ElementShapeFaces *sourceElementShapeFaces = source.getElementShapeFaces(sourceIndex);
		if (!sourceElementShapeFaces)
		{
			// no shape is used for nodal element:xi values when element is not read
			// from the same file, but could in future be used for reading field
			// definitions without shape. Must find a matching global element.
			if (targetIndex < 0)
			{
				display_message(ERROR_MESSAGE, "%d-D element %d is not found in global mesh. "
					"Element must exist before referencing it in embedded locations.",
					this->dimension, identifier);
				result = false;
				break;
			}
		}
		else if (targetIndex >= 0)
		{
			const ElementShapeFaces *targetElementShapeFaces = this->getElementShapeFaces(targetIndex);
			if (!targetElementShapeFaces)
			{
				display_message(ERROR_MESSAGE, "FE_mesh::canMerge.  Target %d-D element %d missing ElementShapeFaces",
					this->dimension, identifier);
				result = false;
				break;
			}
			if (sourceElementShapeFaces->getElementShape() != targetElementShapeFaces->getElementShape())
			{
				display_message(ERROR_MESSAGE, "FE_mesh::canMerge.  Denying merge of %d-D element %d since it is different shape",
					this->dimension, identifier);
				result = false;
				break;
			}
			const int faceCount = sourceElementShapeFaces->getFaceCount();
			if (faceCount > 0)
			{
				if (!(source.faceMesh && this->faceMesh))
				{
					display_message(ERROR_MESSAGE, "FE_mesh::canMerge.  %d-D mesh missing face meshes",
						this->dimension);
					result = false;
					break;
				}
				const DsLabelIndex *sourceFaces = sourceElementShapeFaces->getElementFaces(sourceIndex);
				if (sourceFaces)
				{
					const DsLabelIndex *targetFaces = targetElementShapeFaces->getElementFaces(targetIndex);
					if (targetFaces)
					{
						// check faces refer to same element identifier if both specified
						for (int i = 0; i < faceCount; ++i)
						{
							if ((sourceFaces[i] >= 0) && (targetFaces[i] >= 0) &&
								(source.faceMesh->labels.getIdentifier(sourceFaces[i]) != this->faceMesh->labels.getIdentifier(targetFaces[i])))
							{
								result = false;
								break;
							}
						}
						if (!result)
						{
							display_message(ERROR_MESSAGE, "FE_mesh::canMerge.  Denying merge of %d-D element %d since it has different faces",
								this->dimension, identifier);
							break;
						}
					}
				}
			}
		}
	}
	cmzn::Deaccess(iter);
	return result;
}

namespace {

struct MeshFieldComponentMergeData
{
	FE_field *sourceField;
	FE_field *targetField;
	int componentNumber; // starting at 0
	FE_mesh_field_template *sourceMFT;
	FE_mesh_field_template *targetMFT; // or NULL if none
	FE_mesh_field_template *finalMFT; // to be determined
};

struct IteratorMeshFieldComponentMergeData
{
	const FE_mesh *sourceMesh;
	FE_mesh *targetMesh;
	std::vector<MeshFieldComponentMergeData> &fieldComponentMergeData;

	IteratorMeshFieldComponentMergeData(const FE_mesh *sourceMeshIn, FE_mesh *targetMeshIn,
			std::vector<MeshFieldComponentMergeData> &fieldComponentMergeDataIn) :
		sourceMesh(sourceMeshIn),
		targetMesh(targetMeshIn),
		fieldComponentMergeData(fieldComponentMergeDataIn)
	{
	}
};

int FE_field_addMeshFieldComponentMergeData(struct FE_field *sourceField, void *mergeDataVoid)
{
	IteratorMeshFieldComponentMergeData *mergeData = static_cast<IteratorMeshFieldComponentMergeData *>(mergeDataVoid);
	FE_mesh_field_data *sourceMeshFieldData = sourceField->getMeshFieldData(mergeData->sourceMesh);
	if (!sourceMeshFieldData)
		return 1; // field not defined on source mesh, so nothing to merge
	FE_region *target_FE_region = mergeData->targetMesh->get_FE_region();
	struct FE_field *targetField = FE_region_get_FE_field_from_name(target_FE_region, get_FE_field_name(sourceField));
	if (!targetField)
	{
		display_message(ERROR_MESSAGE,
			"FE_field_addMeshFieldComponentMergeData.  No target field for source field");
		return 0;
	}
	FE_mesh_field_data *targetMeshFieldData = targetField->getMeshFieldData(mergeData->targetMesh); // can be 0
	const int componentCount = get_FE_field_number_of_components(sourceField);
	for (int c = 0; c < componentCount; ++c)
	{
		MeshFieldComponentMergeData componentMergeData =
		{
			sourceField,
			targetField,
			c,
			sourceMeshFieldData->getComponentMeshfieldtemplate(c),
			targetMeshFieldData ? targetMeshFieldData->getComponentMeshfieldtemplate(c) : 0,
			/*finalMFT*/0
		};
		mergeData->fieldComponentMergeData.push_back(componentMergeData);
	}
	return 1;
}

}

/**
 * First part of mesh merge. Merge the elements and faces of source mesh.
 * Do this before merging nodes so that elements exist for defining
 * embedded element:xi node fields.
 * Assumes FE_mesh::canMerge() has been called and returned true.
 * Assumes Face mesh mergePart1Elements has been called and returned true.
 * Assumes FE_region change cache is on.
 * @param source  Source mesh to merge.
 * @return  True on success, false on failure.
 */
bool FE_mesh::mergePart1Elements(const FE_mesh &source)
{
	if (source.dimension != this->dimension)
		return false;
	if (!this->fe_region)
		return false;
	if ((this->dimension > 1) && ((!this->faceMesh) || (!source.faceMesh)))
	{
		display_message(ERROR_MESSAGE, "FE_mesh::merge.  Missing face mesh(es)");
		return false;
	}

	bool result = true;
	// note using a label iterator means elements are merged in identifier order, currently
	DsLabelIterator *iter = source.labels.createLabelIterator();
	if (!iter)
		return false;
	DsLabelIndex sourceElementIndex;
	while ((sourceElementIndex = iter->nextIndex()) >= 0)
	{
		const ElementShapeFaces *sourceElementShapeFaces = source.getElementShapeFaces(sourceElementIndex);
		const DsLabelIdentifier identifier = source.getElementIdentifier(sourceElementIndex);
		DsLabelIndex targetElementIndex = this->labels.findLabelByIdentifier(identifier);
		if (targetElementIndex < 0)
		{
			targetElementIndex = this->createElement(identifier);
			if (targetElementIndex < 0)
			{
				display_message(ERROR_MESSAGE, "FE_mesh::merge.  Failed to create %d-D mesh element %d",
					this->dimension, identifier);
				result = false;
				break;
			}
			this->changeLog->setIndexChange(targetElementIndex, DS_LABEL_CHANGE_TYPE_ADD);
			// canMerge() should have ensured we have sourceElementShapeFaces
			if (!sourceElementShapeFaces)
			{
				display_message(ERROR_MESSAGE, "FE_mesh::merge.  Missing shape for %d-D mesh element %d",
					this->dimension, identifier);
				result = false;
				break;
			}
			if (!this->setElementShape(targetElementIndex, sourceElementShapeFaces->getElementShape()))
			{
				display_message(ERROR_MESSAGE, "FE_mesh::merge.  Failed to set shape for %d-D mesh element %d",
					this->dimension, identifier);
				this->destroyElementPrivate(targetElementIndex);
				result = false;
				break;
			}
		}
		else
		{
			// this is conservative:
			this->changeLog->setIndexChange(targetElementIndex, DS_LABEL_CHANGE_TYPE_DEFINITION | DS_LABEL_CHANGE_TYPE_RELATED);
		}
		if (sourceElementShapeFaces)
		{
			const int faceCount = sourceElementShapeFaces->getFaceCount();
			if (faceCount > 0)
			{
				const DsLabelIndex *sourceFaces = sourceElementShapeFaces->getElementFaces(sourceElementIndex);
				if (sourceFaces)
				{
					ElementShapeFaces *targetElementShapeFaces = this->getElementShapeFaces(targetElementIndex);
					const DsLabelIndex *targetFaces = targetElementShapeFaces->getOrCreateElementFaces(targetElementIndex);
					if (!targetFaces)
					{
						display_message(ERROR_MESSAGE, "FE_mesh::merge.  Failed to create target faces array for %d-D mesh element %d",
							this->dimension, identifier);
						result = false;
						break;
					}
					for (int i = 0; i < faceCount; ++i)
					{
						if (sourceFaces[i] != DS_LABEL_INDEX_INVALID)
						{
							const DsLabelIdentifier sourceFaceIdentifier = source.faceMesh->getElementIdentifier(sourceFaces[i]);
							const DsLabelIndex faceIndex = this->faceMesh->labels.findLabelByIdentifier(sourceFaceIdentifier);
							if (faceIndex < 0)
							{
								display_message(ERROR_MESSAGE, "FE_mesh::merge.  Missing target face");
								result = false;
								break;
							}
							// must call setElementFace to also set up parent lists
							if (CMZN_OK != this->setElementFace(targetElementIndex, i, faceIndex))
							{
								display_message(ERROR_MESSAGE, "FE_mesh::merge.  Failed to set new face");
								result = false;
							}
						}
					}
					if (!result)
						break;
				}
			}
		}
	}
	cmzn::Deaccess(iter);
	return result;
}

/**
 * Second part of mesh merge. Merge field definitions from source mesh,
 * i.e. shared mesh field templates.
 * Do this after merging nodes so that the exist for defining node-based
 * element field parameter mappings.
 * Assumes FE_mesh::mergePart1Elements has been called and returned true.
 * Assumes FE_region change cache is on.
 * @param source  Source mesh to merge.
 * @return  True on success, false on failure.
 */
bool FE_mesh::mergePart2Fields(const FE_mesh &source)
{
	if (source.dimension != this->dimension)
		return false;

	bool result = true;
	// make table of source to merged target EFT data index
	std::vector<FE_mesh_field_template::EFTIndexType> sourceToTargetEFTDataIndex;
	for (int i = 0; i < source.elementFieldTemplateDataCount; ++i)
	{
		FE_mesh_field_template::EFTIndexType targetEftIndex = FE_mesh_field_template::ELEMENT_FIELD_TEMPLATE_DATA_INDEX_INVALID;
		if (source.elementFieldTemplateData[i])
		{
			// must clone otherwise attempting to merge into a different mesh
			FE_element_field_template *sourceEFT = source.elementFieldTemplateData[i]->getElementfieldtemplate()->cloneForNewMesh(this);
			FE_element_field_template *targetEFT = this->mergeElementfieldtemplate(sourceEFT);
			if (0 == targetEFT)
			{
				display_message(ERROR_MESSAGE, "FE_mesh::merge.  Failed to merge element field template");
				return false;
			}
			targetEftIndex = static_cast<FE_mesh_field_template::EFTIndexType>(targetEFT->getIndexInMesh());
			FE_element_field_template::deaccess(targetEFT);
			FE_element_field_template::deaccess(sourceEFT);
		}
		sourceToTargetEFTDataIndex.push_back(targetEftIndex);
	}

	// make table of source, target field component MFTs
	std::vector<MeshFieldComponentMergeData> fieldComponentMergeData;
	{
		IteratorMeshFieldComponentMergeData iteratorMergeData(&source, this, fieldComponentMergeData);
		if (!FE_region_for_each_FE_field(source.fe_region, FE_field_addMeshFieldComponentMergeData,
			static_cast<void *>(&iteratorMergeData)))
		{
			display_message(ERROR_MESSAGE, "FE_mesh::merge.  Failed to create mesh field template map");
			return false;
		}
	}

	const int fieldComponentCount = static_cast<int>(fieldComponentMergeData.size());
	for (int fc = 0; fc < fieldComponentCount; ++fc)
	{
		MeshFieldComponentMergeData& fieldComponentData = fieldComponentMergeData[fc];
		// following will be set if previous field component had same sourceMFT and targetMFT
		FE_mesh_field_template *finalMFT = fieldComponentData.finalMFT;
		if (finalMFT)
		{
			finalMFT->access();
		}
		else
		{
			// determine finalMFT, and whether sourceMFT must be merged into it
			bool mergeSourceMFT = false;
			if (fieldComponentData.targetMFT)
			{
				// can use current targetMFT if it is a superset of sourceMFT
				if (fieldComponentData.targetMFT->matchesWithEFTIndexMap(*fieldComponentData.sourceMFT,
					sourceToTargetEFTDataIndex, /*superset*/true))
				{
					finalMFT = fieldComponentData.targetMFT->access();
				}
				else
				{
					// could try to find matching subset of other target MFTs, but expensive and unlikely
					mergeSourceMFT = true;
					// if all fields using targetMFT are being updated by same sourceMFT, can merge into it
					// number using targetMFT is its access count internally
					int usageCount = 1;
					for (int fc2 = fc + 1; fc2 < fieldComponentCount; ++fc2)
					{
						MeshFieldComponentMergeData& fieldComponentData2 = fieldComponentMergeData[fc2];
						if ((fieldComponentData2.sourceMFT == fieldComponentData.sourceMFT) &&
							(fieldComponentData2.targetMFT == fieldComponentData.targetMFT))
						{
							++usageCount;
						}
					}
					if (usageCount == fieldComponentData.targetMFT->getFieldComponentUsageCount())
					{
						finalMFT = fieldComponentData.targetMFT->access();
					}
					else
					{
						finalMFT = this->cloneMeshFieldTemplate(fieldComponentData.targetMFT);
						if (!finalMFT)
						{
							display_message(ERROR_MESSAGE, "FE_mesh::merge.  Failed to clone mesh field template");
							return false;
						}
					}
				}
			}
			else
			{
				// search for exact match in existing mesh field templates
				for (std::list<FE_mesh_field_template*>::iterator iter = this->meshFieldTemplates.begin();
					iter != this->meshFieldTemplates.end(); ++iter)
				{
					FE_mesh_field_template *mft = *iter;
					if (mft->matchesWithEFTIndexMap(*fieldComponentData.sourceMFT, sourceToTargetEFTDataIndex, /*superset*/false))
					{
						finalMFT = mft->access();
						break;
					}
				}
				if (!finalMFT)
				{
					mergeSourceMFT = true;
					finalMFT = this->createBlankMeshFieldTemplate();
					if (!finalMFT)
					{
						display_message(ERROR_MESSAGE, "FE_mesh::merge.  Failed to create mesh field template");
						return false;
					}
				}
			}
			if (mergeSourceMFT)
			{
				if (!finalMFT->mergeWithEFTIndexMap(*fieldComponentData.sourceMFT, sourceToTargetEFTDataIndex))
				{
					display_message(ERROR_MESSAGE, "FE_mesh::merge.  Failed to merge mesh field template");
					FE_mesh_field_template::deaccess(finalMFT);
					return false;
				}
			}
			// set finalMFT for this and all subsequent field components with same sourceMFT and targetMFT
			fieldComponentData.finalMFT = finalMFT;
			for (int fc2 = fc + 1; fc2 < fieldComponentCount; ++fc2)
			{
				MeshFieldComponentMergeData& fieldComponentData2 = fieldComponentMergeData[fc2];
				if ((fieldComponentData2.sourceMFT == fieldComponentData.sourceMFT) &&
					(fieldComponentData2.targetMFT == fieldComponentData.targetMFT)) // can be NULL
				{
					fieldComponentData2.finalMFT = finalMFT;
				}
			}
		}
		FE_mesh_field_data *sourceMeshFieldData = fieldComponentData.sourceField->getMeshFieldData(&source); // must exist
		FE_mesh_field_data *targetMeshFieldData = fieldComponentData.targetField->getMeshFieldData(this);
		if (!targetMeshFieldData)
		{
			targetMeshFieldData = fieldComponentData.targetField->createMeshFieldData(this);
			if (!targetMeshFieldData)
			{
				display_message(ERROR_MESSAGE, "FE_mesh::merge.  Failed to create mesh field data");
				result = false;
			}
		}
		if (result)
		{
			if (finalMFT != fieldComponentData.targetMFT)
				targetMeshFieldData->setComponentMeshfieldtemplate(fieldComponentData.componentNumber, finalMFT);
		}
		FE_mesh_field_template::deaccess(finalMFT);
		if (!result)
			break;
		FE_mesh_field_data::ComponentBase *sourceComponent = sourceMeshFieldData->components[fieldComponentData.componentNumber];
		FE_mesh_field_data::ComponentBase *targetComponent = targetMeshFieldData->components[fieldComponentData.componentNumber];
		// future optimisation: avoid following if no source EFTs have element values
		if (!targetComponent->mergeElementValues(sourceComponent))
		{
			display_message(ERROR_MESSAGE, "FE_mesh::merge.  Failed to merge element values");
			result = false;
			break;
		}
		if (0 == fieldComponentData.componentNumber)
		{
			this->fe_region->FE_field_change(fieldComponentData.targetField, CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field));
		}
	}

	// 3. merge shared element local-to-global node map and scale factors

	for (int sourceEFTIndex = 0; sourceEFTIndex < source.elementFieldTemplateDataCount; ++sourceEFTIndex)
	{
		const FE_mesh_element_field_template_data *sourceEFTData = source.elementFieldTemplateData[sourceEFTIndex];
		if ((sourceEFTData) && sourceEFTData->hasElementVaryingData())
		{
			FE_mesh_element_field_template_data *targetEFTData = this->elementFieldTemplateData[sourceToTargetEFTDataIndex[sourceEFTIndex]];
			if (!targetEFTData)
			{
				display_message(ERROR_MESSAGE, "FE_mesh::merge.  Missing target element field template data");
				result = false;
				break;
			}
			if (!targetEFTData->mergeElementVaryingData(*sourceEFTData))
			{
				display_message(ERROR_MESSAGE, "FE_mesh::merge.  Failed to merge element local-to-global maps");
				result = false;
				break;
			}
			// simplest to mark all fields as changed as they may share local nodes and scale factors
			// can optimise in future
			this->fe_region->FE_field_all_change(CHANGE_LOG_RELATED_OBJECT_CHANGED(FE_field));
		}
	}

	return result;
}

FE_mesh_field_data::FE_mesh_field_data(FE_field *fieldIn, ComponentBase **componentsIn) :
	field(fieldIn),
	componentCount(fieldIn->getNumberOfComponents()),
	valueType(fieldIn->getValueType()),
	components(componentsIn) // takes ownership of passed-in array
{
}

FE_mesh_field_data *FE_mesh_field_data::create(FE_field *field, FE_mesh *mesh)
{
	if (!(field && mesh))
		return 0;
	FE_mesh_field_data *meshFieldData = 0;
	FE_mesh_field_template *blankMeshFieldTemplate = mesh->getOrCreateBlankMeshFieldTemplate();
	const int componentCount = get_FE_field_number_of_components(field);
	ComponentBase **components = new ComponentBase*[componentCount];
	if (components)
	{
		for (int c = 0; c < componentCount; ++c)
			components[c] = 0;
	}
	if (blankMeshFieldTemplate && components)
	{
		bool success = true;
		for (int c = 0; c < componentCount; ++c)
		{
			const Value_type valueType = get_FE_field_value_type(field);
			switch (valueType)
			{
			case FE_VALUE_VALUE:
				components[c] = new Component<FE_value>(blankMeshFieldTemplate);
				break;
			case INT_VALUE:
				components[c] = new Component<int>(blankMeshFieldTemplate);
				break;
			case STRING_VALUE:
			{
				FE_field_type feFieldType = get_FE_field_FE_field_type(field);
				if ((feFieldType == CONSTANT_FE_FIELD) || (feFieldType == INDEXED_FE_FIELD))
				{
					components[c] = new ComponentConstant(blankMeshFieldTemplate);
				}
				else
				{
					display_message(ERROR_MESSAGE, "FE_mesh_field_data::create.  String type is only implemented for constant and indexed field");
					success = false;
				}
			}	break;
			default:
				display_message(ERROR_MESSAGE, "FE_mesh_field_data::create.  Unsupported value type");
				success = false;
				break;
			}
			if (success && (!components[c]))
			{
				if (success)
				{
					display_message(ERROR_MESSAGE, "FE_mesh_field_data::create.  Failed to create component");
					success = false;
				}
			}
		}
		if (success)
			meshFieldData = new FE_mesh_field_data(field, components);
	}
	FE_mesh_field_template::deaccess(blankMeshFieldTemplate);
	if (!meshFieldData)
	{
		if (components)
		{
			for (int c = 0; c < componentCount; ++c)
				delete components[c];
			delete[] components;
		}
	}
	return meshFieldData;
}
