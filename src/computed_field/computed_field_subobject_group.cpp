/***************************************************************************//**
 * FILE : computed_field_subobject_group.cpp
 *
 * Implements region sub object groups, e.g. node group, element group.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdlib.h>
#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/node.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/fieldsubobjectgroup.h"
#include "computed_field/computed_field.h"
#if defined (USE_OPENCASCADE)
#include "opencmiss/zinc/fieldcad.h"
#endif /* defined (USE_OPENCASCADE) */

#include "computed_field/computed_field_subobject_group.hpp"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/field_cache.hpp"
#include "computed_field/field_module.hpp"
#include "finite_element/finite_element_region.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"
#include "mesh/cmiss_element_private.hpp"
#include "mesh/cmiss_node_private.hpp"
#include "region/cmiss_region.hpp"
#if defined (USE_OPENCASCADE)
#include "cad/computed_field_cad_topology.h"
#include "cad/element_identifier.h"
#endif /* defined (USE_OPENCASCADE) */

Computed_field_subobject_group::~Computed_field_subobject_group()
{
	if (this->ownerGroup)
		display_message(ERROR_MESSAGE, "Computed_field_subobject_group %s destroyed with non-zero ownerGroup",
			this->field ? this->field->name : "?");
}

namespace {

class Computed_field_sub_group_object_package : public Computed_field_type_package
{
public:
	cmzn_region *root_region;

	Computed_field_sub_group_object_package(cmzn_region *root_region)
		: root_region(root_region)
	{
		ACCESS(cmzn_region)(root_region);
	}
	
	~Computed_field_sub_group_object_package()
	{
		DEACCESS(cmzn_region)(&root_region);
	}
};

struct cmzn_node_field_is_true_iterator_data
{
	cmzn_fieldcache_id cache;
	cmzn_field_id field;
};

int cmzn_node_field_is_true_iterator(cmzn_node_id node, void *data_void)
{
	cmzn_node_field_is_true_iterator_data *data =
		static_cast<cmzn_node_field_is_true_iterator_data *>(data_void);
	cmzn_fieldcache_set_node(data->cache, node);
	return cmzn_field_evaluate_boolean(data->field, data->cache) ? 1 : 0;
}

} // anonymous namespace

Computed_field_element_group *Computed_field_element_group::create(FE_mesh *fe_mesh_in)
{
	Computed_field_element_group *element_group = 0;
	if (fe_mesh_in)
	{
		DsLabelsGroup *labelsGroup = fe_mesh_in->createLabelsGroup();
		if (labelsGroup)
		{
			element_group = new Computed_field_element_group(fe_mesh_in, labelsGroup);
			cmzn::Deaccess(labelsGroup);
		}
	}
	return element_group;
}

int Computed_field_element_group::addObject(cmzn_element *object)
{
	if (!isElementCompatible(object))
		return CMZN_ERROR_ARGUMENT;
	const bool handleSubelements =
		(this->getSubobjectHandlingMode() == CMZN_FIELD_GROUP_SUBELEMENT_HANDLING_MODE_FULL);
	if (handleSubelements)
		this->beginChange();
	int return_code = this->labelsGroup->setIndex(get_FE_element_index(object), true);
	if (CMZN_OK == return_code)
	{
		this->invalidateIterators();
		change_detail.changeAdd();
		update();
	}
	if (handleSubelements)
	{
		if ((CMZN_OK == return_code) || (CMZN_ERROR_ALREADY_EXISTS == return_code))
		{
			int subresult = this->addSubelements(object);
			if (CMZN_OK != subresult)
				return_code = subresult;
		}
		this->endChange();
	}
	return return_code;
}

// GRC future: identical to Computed_field_node_group::addObjectsInIdentifierRange -> move to common base class
int Computed_field_element_group::addObjectsInIdentifierRange(DsLabelIdentifier first, DsLabelIdentifier last)
{
	DsLabels *labels = this->labelsGroup->getLabels();
	if ((first > last) || (!labels))
		return CMZN_ERROR_ARGUMENT;
	int return_code = CMZN_OK;
	DsLabelIndex index;
	DsLabelIdentifier identifier;
	const DsLabelIndex indexSize = labels->getIndexSize();
	if (((last - first) > indexSize) ||
		((!labels->isContiguous()) && ((last - first) > (indexSize/10))))
	{
		for (index = 0; index < indexSize; ++index)
		{
			identifier = labels->getIdentifier(index);
			// invalid identifier == deleted element; skip
			if ((DS_LABEL_IDENTIFIER_INVALID != identifier) &&
				(identifier >= first) && (identifier <= last))
			{
				const int result = this->labelsGroup->setIndex(index, true);
				if ((result != CMZN_OK) && (result != CMZN_ERROR_ALREADY_EXISTS))
				{
					return_code = result;
					break;
				}
			}
		}
	}
	else
	{
		for (identifier = first; identifier <= last; ++identifier)
		{
			index = labels->findLabelByIdentifier(identifier);
			if (DS_LABEL_INDEX_INVALID != index)
			{
				const int result = this->labelsGroup->setIndex(index, true);
				if ((result != CMZN_OK) && (result != CMZN_ERROR_ALREADY_EXISTS))
				{
					return_code = result;
					break;
				}
			}
		}
	}
	return return_code;
}

int Computed_field_element_group::addElementsConditional(cmzn_field_id conditional_field)
{
	if ((!conditional_field) || (conditional_field->manager != this->field->manager))
		return CMZN_ERROR_ARGUMENT;
	bool isEmptyGroup;
	Computed_field_element_group *otherElementGroup =
		this->getConditionalElementGroup(conditional_field, isEmptyGroup);
	if (isEmptyGroup)
		return CMZN_OK;
	int return_code = CMZN_OK;
	this->beginChange();
	cmzn_fieldcache *cache = 0;
	const int oldSize = this->getSize();
	const bool handleSubelements =
		(this->getSubobjectHandlingMode() == CMZN_FIELD_GROUP_SUBELEMENT_HANDLING_MODE_FULL);
	cmzn_elementiterator *iter;
	if (otherElementGroup)
		iter = otherElementGroup->createElementiterator(); // could handle this case very efficiently with logical OR
	else
	{
		iter = this->fe_mesh->createElementiterator();
		cache = new cmzn_fieldcache(FE_region_get_cmzn_region(this->fe_mesh->get_FE_region()));
		if (!cache)
			return_code = CMZN_ERROR_MEMORY;
	}
	if (!iter)
		return_code = CMZN_ERROR_MEMORY;
	if (CMZN_OK == return_code)
	{
		cmzn_element_id element = 0;
		while (0 != (element = iter->nextElement()))
		{
			if (cache)
			{
				cache->setElement(element);
				if (!cmzn_field_evaluate_boolean(conditional_field, cache))
					continue;
			}
			const int result = this->labelsGroup->setIndex(get_FE_element_index(element), true);
			if ((result != CMZN_OK) && (result != CMZN_ERROR_ALREADY_EXISTS))
			{
				return_code = result;
				break;
			}
			if (handleSubelements)
			{
				int subresult = this->addSubelements(element);
				if (subresult != CMZN_OK)
				{
					return_code = subresult;
					break;
				}
			}
		}
	}
	cmzn::Deaccess(iter);
	const int newSize = this->getSize();
	if (newSize != oldSize)
	{
		this->invalidateIterators();
		change_detail.changeAdd();
		update();
	}
	cmzn_fieldcache_destroy(&cache);
	this->endChange();
	return return_code;
}

int Computed_field_element_group::removeObject(cmzn_element *object)
{
	if (!isElementCompatible(object))
		return CMZN_ERROR_ARGUMENT;
	const bool handleSubelements =
		(this->getSubobjectHandlingMode() == CMZN_FIELD_GROUP_SUBELEMENT_HANDLING_MODE_FULL);
	if (handleSubelements)
		this->beginChange();
	int return_code = this->labelsGroup->setIndex(get_FE_element_index(object), false);
	if (CMZN_OK == return_code)
	{
		this->invalidateIterators();
		change_detail.changeRemove();
		update();
	}
	if (handleSubelements)
	{
		if ((CMZN_OK == return_code) || (CMZN_ERROR_NOT_FOUND == return_code))
		{
			int subresult = this->removeSubelements(object);
			if (CMZN_OK != subresult)
				return_code = subresult;
		}
		this->endChange();
	}
	return return_code;
};

int Computed_field_element_group::removeElementsConditional(cmzn_field_id conditional_field)
{
	if ((!conditional_field) || (conditional_field->manager != this->field->manager))
		return CMZN_ERROR_ARGUMENT;
	bool isEmptyGroup;
	Computed_field_element_group *otherElementGroup =
		this->getConditionalElementGroup(conditional_field, isEmptyGroup);
	if (isEmptyGroup)
		return CMZN_OK;
	if (otherElementGroup == this)
		return this->clear();
	const int oldSize = this->getSize();
	if (oldSize == 0)
		return CMZN_OK;
	int return_code = CMZN_OK;
	this->beginChange();
	cmzn_fieldcache *cache = 0;
	const bool handleSubelements =
		(this->getSubobjectHandlingMode() == CMZN_FIELD_GROUP_SUBELEMENT_HANDLING_MODE_FULL);
	cmzn_elementiterator *iter;
	if (otherElementGroup)
		iter = otherElementGroup->createElementiterator(); // could handle this case very efficiently with logical AND of complement
	else
	{
		iter = this->fe_mesh->createElementiterator();
		cache = new cmzn_fieldcache(FE_region_get_cmzn_region(this->fe_mesh->get_FE_region()));
		if (!cache)
			return_code = CMZN_ERROR_MEMORY;
	}
	if (!iter)
		return_code = CMZN_ERROR_MEMORY;
	DsLabelsGroup *removedLabelsGroup = 0;
	if (handleSubelements)
	{
		removedLabelsGroup = this->fe_mesh->createLabelsGroup();
		if (!removedLabelsGroup)
			return_code = CMZN_ERROR_MEMORY;
	}
	if (CMZN_OK == return_code)
	{
		cmzn_element_id element = 0;
		while (0 != (element = iter->nextElement()))
		{
			if (cache)
			{
				cache->setElement(element);
				if (!cmzn_field_evaluate_boolean(conditional_field, cache))
					continue;
			}
			const DsLabelIndex index = get_FE_element_index(element);
			const int result = this->labelsGroup->setIndex(index, false);
			if ((result != CMZN_OK) && (result != CMZN_ERROR_NOT_FOUND))
			{
				return_code = result;
				break;
			}
			if (handleSubelements)
			{
				int subresult = removedLabelsGroup->setIndex(index, true);
				if (CMZN_OK != subresult)
				{
					return_code = subresult;
					break;
				}
			}
		}
		if (handleSubelements && (CMZN_OK == return_code))
			return_code = this->removeSubelementsList(*removedLabelsGroup);
	}
	cmzn::Deaccess(iter);
	const int newSize = this->getSize();
	if (newSize != oldSize)
	{
		this->invalidateIterators();
		change_detail.changeRemove();
		update();
	}
	cmzn_fieldcache_destroy(&cache);
	cmzn::Deaccess(removedLabelsGroup);
	this->endChange();
	return return_code;
}

int Computed_field_element_group::clear()
{
	int return_code = CMZN_OK;
	if (0 < this->getSize())
	{
		this->beginChange();
		this->invalidateIterators();
		if (this->getSubobjectHandlingMode() == CMZN_FIELD_GROUP_SUBELEMENT_HANDLING_MODE_FULL)
		{
			DsLabelsGroup *tmpLabelsGroup = DsLabelsGroup::create(this->labelsGroup->getLabels());
			if (tmpLabelsGroup)
			{
				tmpLabelsGroup->swap(*this->labelsGroup);
				this->removeSubelementsList(*tmpLabelsGroup);
				cmzn::Deaccess(tmpLabelsGroup);
			}
			else
			{
				display_message(INFORMATION_MESSAGE, "Computed_field_element_group::clear.  Failed to handle subelements");
				this->labelsGroup->clear();
				return_code = CMZN_ERROR_MEMORY;
			}
		}
		else
			this->labelsGroup->clear();
		change_detail.changeRemove();
		update();
		this->endChange();
	}
	return return_code;
};

void Computed_field_element_group::write_btree_statistics() const
{
	display_message(INFORMATION_MESSAGE, "%d-D elements:\n", this->fe_mesh->getDimension());
	display_message(INFORMATION_MESSAGE, "  %d labels in bool array\n", this->labelsGroup->getSize());
}

int Computed_field_element_group::addElementFaces(cmzn_element_id parent)
{
	if (!isParentElementCompatible(parent))
		return CMZN_ERROR_ARGUMENT;
	const DsLabelIndex parentIndex = get_FE_element_index(parent);
	const FE_mesh::ElementShapeFaces *elementShapeFaces =
		this->fe_mesh->getParentMesh()->getElementShapeFacesConst(parentIndex);
	if (!elementShapeFaces)
		return CMZN_ERROR_ARGUMENT;
	const int faceCount = elementShapeFaces->getFaceCount();
	if (0 == faceCount)
		return CMZN_OK;
	const DsLabelIndex *faces = elementShapeFaces->getElementFaces(parentIndex);
	if (!faces)
		return CMZN_OK;
	int return_code = CMZN_OK;
	this->beginChange();
	for (int i = 0; i < faceCount; ++i)
	{
		if (faces[i] >= 0)
		{
			struct FE_element *face = this->fe_mesh->getElement(faces[i]);
			if (face)
			{
				const int result = this->addObject(face);
				if ((result != CMZN_OK) && (result != CMZN_ERROR_ALREADY_EXISTS))
				{
					return_code = result;
					break;
				}
			}
		}
	}
	this->endChange();
	return return_code;
}

int Computed_field_element_group::addSubelements(cmzn_element_id element)
{
	int return_code = CMZN_OK;
	FE_mesh *faceMesh = this->fe_mesh->getFaceMesh();
	if (faceMesh)
	{
		Computed_field_element_group *faceElementGroup =
			this->ownerGroup->getElementGroupPrivate(faceMesh, /*create*/true);
		if (faceElementGroup)
			return_code = faceElementGroup->addElementFacesRecursive(*this->fe_mesh, get_FE_element_index(element));
		else
			return_code = CMZN_ERROR_GENERAL;
	}
	if (CMZN_OK == return_code)
	{
		Computed_field_node_group *nodeGroup = this->ownerGroup->getNodeGroupPrivate(CMZN_FIELD_DOMAIN_TYPE_NODES, /*create*/true);
		if (nodeGroup)
			return_code = nodeGroup->addElementNodes(element);
		else
			return_code = CMZN_ERROR_GENERAL;
	}
	return return_code;
}

int Computed_field_element_group::removeSubelements(cmzn_element_id element)
{
	int return_code = CMZN_OK;
	const DsLabelIndex elementIndex = get_FE_element_index(element);
	FE_mesh *faceMesh = this->fe_mesh->getFaceMesh();
	if (faceMesh)
	{
		Computed_field_element_group *faceElementGroup =
			this->ownerGroup->getElementGroupPrivate(faceMesh);
		if (faceElementGroup)
			return_code = faceElementGroup->removeElementFacesRecursive(*this, elementIndex);
	}
	if (CMZN_OK == return_code)
	{
		Computed_field_node_group *nodeGroup = this->ownerGroup->getNodeGroupPrivate(CMZN_FIELD_DOMAIN_TYPE_NODES);
		if (nodeGroup)
		{
			DsLabelsGroup *removeNodeLabelsGroup = nodeGroup->createRelatedNodeLabelsGroup();
			if (removeNodeLabelsGroup)
			{
				// add nodes from this element, but remove nodes from neighbours still in group
				return_code = cmzn_element_add_nodes_to_labels_group(element, *removeNodeLabelsGroup);
				if (CMZN_OK == return_code)
				{
					if (faceMesh)
					{
						const FE_mesh::ElementShapeFaces *elementShapeFaces = this->fe_mesh->getElementShapeFacesConst(elementIndex);
						const DsLabelIndex *faces;
						if ((elementShapeFaces) && (faces = elementShapeFaces->getElementFaces(elementIndex)))
						{
							const int faceCount = elementShapeFaces->getFaceCount();
							for (int f = 0; f < faceCount; ++f)
							{
								const DsLabelIndex *parents;
								const int parentsCount = faceMesh->getElementParents(faces[f], parents);
								for (int p = 0; p < parentsCount; ++p)
									if ((parents[p] != elementIndex) && this->containsIndex(parents[p]))
									{
										return_code = cmzn_element_remove_nodes_from_labels_group(this->fe_mesh->getElement(parents[p]), *removeNodeLabelsGroup);
										if (CMZN_OK != return_code)
											break;
									}
							}
						}
					}
					else if (1 == this->fe_mesh->getDimension())
					{
						// remove nodes used by remaining elements = expensive
						cmzn_elementiterator *iter = this->createElementiterator();
						if (!iter)
							return_code = CMZN_ERROR_MEMORY;
						cmzn_element *tmpElement;
						while ((0 != (tmpElement = iter->nextElement())) && (CMZN_OK == return_code))
							return_code = cmzn_element_remove_nodes_from_labels_group(tmpElement, *removeNodeLabelsGroup);
						cmzn::Deaccess(iter);
					}
					if (CMZN_OK == return_code)
						return_code = nodeGroup->removeNodesInLabelsGroup(*removeNodeLabelsGroup);
					cmzn::Deaccess(removeNodeLabelsGroup);
				}
			}
			else
				return_code = CMZN_ERROR_MEMORY;
		}
	}
	return return_code;
}

int Computed_field_element_group::removeSubelementsList(DsLabelsGroup &removedlabelsGroup)
{
	int return_code = CMZN_OK;
	cmzn_elementiterator *iter;
	cmzn_element *element;
	FE_mesh *faceMesh = this->fe_mesh->getFaceMesh();
	if (faceMesh)
	{
		Computed_field_element_group *faceElementGroup =
			this->ownerGroup->getElementGroupPrivate(faceMesh);
		if (faceElementGroup)
		{
			iter = this->fe_mesh->createElementiterator(&removedlabelsGroup);
			if (!iter)
				return_code = CMZN_ERROR_MEMORY;
			while ((0 != (element = cmzn_elementiterator_next_non_access(iter))) && (CMZN_OK == return_code))
				return_code = faceElementGroup->removeElementFacesRecursive(*this, get_FE_element_index(element));
			cmzn::Deaccess(iter);
		}
	}
	if (CMZN_OK == return_code)
	{
		Computed_field_node_group *nodeGroup = this->ownerGroup->getNodeGroupPrivate(CMZN_FIELD_DOMAIN_TYPE_NODES);
		if (nodeGroup)
		{
			DsLabelsGroup *removeNodeLabelsGroup = nodeGroup->createRelatedNodeLabelsGroup();
			if (removeNodeLabelsGroup)
			{
				// fill list with nodes from removed elements
				iter = this->fe_mesh->createElementiterator(&removedlabelsGroup);
				if (!iter)
					return_code = CMZN_ERROR_MEMORY;
				while ((0 != (element = cmzn_elementiterator_next_non_access(iter))) && (CMZN_OK == return_code))
					return_code = cmzn_element_add_nodes_to_labels_group(element, *removeNodeLabelsGroup);
				cmzn::Deaccess(iter);

				// remove nodes used by remaining elements = expensive
				iter = this->createElementiterator();
				if (!iter)
					return_code = CMZN_ERROR_MEMORY;
				while ((0 != (element = cmzn_elementiterator_next_non_access(iter))) && (CMZN_OK == return_code))
					return_code = cmzn_element_remove_nodes_from_labels_group(element, *removeNodeLabelsGroup);
				cmzn::Deaccess(iter);

				if (CMZN_OK == return_code)
					return_code = nodeGroup->removeNodesInLabelsGroup(*removeNodeLabelsGroup);
				cmzn::Deaccess(removeNodeLabelsGroup);
			}
			else
				return_code = CMZN_ERROR_MEMORY;
		}
	}
	return return_code;
}

int Computed_field_element_group::addElementFacesRecursive(FE_mesh& parentMesh, DsLabelIndex parentIndex)
{
	Computed_field_element_group *faceElementGroup = 0; // for adding faces of faces
	FE_mesh *faceMesh = this->fe_mesh->getFaceMesh();
	if (faceMesh)
	{
		faceElementGroup = this->ownerGroup->getElementGroupPrivate(faceMesh, /*create*/true);
		if (!faceElementGroup)
			return CMZN_ERROR_GENERAL;
	}
	const FE_mesh::ElementShapeFaces *elementShapeFaces =
		this->fe_mesh->getParentMesh()->getElementShapeFacesConst(parentIndex);
	if (!elementShapeFaces)
		return CMZN_ERROR_ARGUMENT;
	const int faceCount = elementShapeFaces->getFaceCount();
	if (0 == faceCount)
		return CMZN_OK;
	const DsLabelIndex *faces = elementShapeFaces->getElementFaces(parentIndex);
	if (!faces)
		return CMZN_OK;
	int return_code = CMZN_OK;
	int numberAdded = 0;
	for (int i = 0; i < faceCount; ++i)
	{
		if (faces[i] >= 0)
		{
			const int result = this->labelsGroup->setIndex(faces[i], true);
			if (CMZN_OK == result)
				++numberAdded;
			else if (CMZN_ERROR_ALREADY_EXISTS != result)
			{
				return_code = result;
				break;
			}
			if (faceElementGroup)
			{
				return_code = faceElementGroup->addElementFacesRecursive(*faceMesh, faces[i]);
				if (CMZN_OK != return_code)
					break;
			}
		}
	}
	if (numberAdded)
	{
		this->invalidateIterators();
		this->change_detail.changeAdd();
		this->update();
	}
	return return_code;
}

int Computed_field_element_group::removeElementFaces(cmzn_element_id parent)
{
	if (!isParentElementCompatible(parent))
		return CMZN_ERROR_ARGUMENT;
	const DsLabelIndex parentIndex = get_FE_element_index(parent);
	const FE_mesh::ElementShapeFaces *elementShapeFaces =
		this->fe_mesh->getParentMesh()->getElementShapeFacesConst(parentIndex);
	if (!elementShapeFaces)
		return CMZN_ERROR_ARGUMENT;
	const int faceCount = elementShapeFaces->getFaceCount();
	if (0 == faceCount)
		return CMZN_OK;
	const DsLabelIndex *faces = elementShapeFaces->getElementFaces(parentIndex);
	if (!faces)
		return CMZN_OK;
	int return_code = CMZN_OK;
	this->beginChange();
	for (int i = 0; i < faceCount; ++i)
	{
		if (faces[i] >= 0)
		{
			struct FE_element *face = this->fe_mesh->getElement(faces[i]);
			if (face)
			{
				const int result = this->removeObject(face);
				if ((result != CMZN_OK) && (result != CMZN_ERROR_NOT_FOUND))
				{
					return_code = result;
					break;
				}
			}
		}
	}
	this->endChange();
	return return_code;
}

int Computed_field_element_group::removeElementFacesRecursive(
	Computed_field_element_group& parentElementGroup, DsLabelIndex parentIndex)
{
	Computed_field_element_group *faceElementGroup = 0;
	FE_mesh *faceMesh = this->fe_mesh->getFaceMesh();
	if (faceMesh)
		faceElementGroup = this->ownerGroup->getElementGroupPrivate(faceMesh, /*create*/false);
	FE_mesh *parentMesh = this->fe_mesh->getParentMesh();
	const FE_mesh::ElementShapeFaces *elementShapeFaces = parentMesh->getElementShapeFacesConst(parentIndex);
	if (!elementShapeFaces)
		return CMZN_ERROR_ARGUMENT;
	const int faceCount = elementShapeFaces->getFaceCount();
	if (0 == faceCount)
		return CMZN_OK;
	const DsLabelIndex *faces = elementShapeFaces->getElementFaces(parentIndex);
	if (!faces)
		return CMZN_OK;
	int return_code = CMZN_OK;
	int numberRemoved = 0;
	for (int i = 0; i < faceCount; ++i)
	{
		if (faces[i] >= 0)
		{
			const DsLabelIndex *parents;
			const int parentsCount = this->fe_mesh->getElementParents(faces[i], parents);
			bool keepFace = false;
			for (int p = 0; p < parentsCount; ++p)
				if (parentElementGroup.labelsGroup->hasIndex(parents[p]))
				{
					keepFace = true;
					break;
				}
			if (keepFace)
				continue;
			const int result = this->labelsGroup->setIndex(faces[i], false);
			if (CMZN_OK == result)
			{
				++numberRemoved;
				if (faceElementGroup)
				{
					return_code = faceElementGroup->removeElementFacesRecursive(*this, faces[i]);
					if (CMZN_OK != return_code)
						break;
				}
			}
			else if (CMZN_ERROR_NOT_FOUND != result)
			{
				return_code = result;
				break;
			}
		}
	}
	if (numberRemoved)
	{
		this->invalidateIterators();
		this->change_detail.changeRemove();
		this->update();
	}
	return return_code;
}

int Computed_field_element_group::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	const Field_location_element_xi *element_xi_location = cache.get_location_element_xi();
	if (element_xi_location)
	{
		RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
		cmzn_element_id element = element_xi_location->get_element();
		valueCache.values[0] = this->containsObject(element) ? 1 : 0;
		return 1;
	}
	return 0;
};

/** remove elements that have been removed from master mesh */
int Computed_field_element_group::check_dependency()
{
	if (field)
	{
		DsLabelsChangeLog *elementChangeLog = this->fe_mesh->getChangeLog();
		if (elementChangeLog)
		{
			const int changeSummary = elementChangeLog->getChangeSummary();
			if (changeSummary & DS_LABEL_CHANGE_TYPE_REMOVE)
			{
				const int oldSize = this->labelsGroup->getSize();
				if (0 < oldSize)
				{
					// special handling for zero mesh size because labels will have been cleared
					// plus more efficient to clear group
					if (0 == this->fe_mesh->getSize())
					{
						this->labelsGroup->clear();
					}
					else
					{
						DsLabelIndex index = -1; // DS_LABEL_INDEX_INVALID
						while (this->labelsGroup->incrementIndex(index))
						{
							if (this->fe_mesh->getElementIdentifier(index) == DS_LABEL_IDENTIFIER_INVALID)
								this->labelsGroup->setIndex(index, false);
						}
					}
					const int newSize = this->labelsGroup->getSize();
					if (newSize != oldSize)
					{
						this->invalidateIterators();
						change_detail.changeRemove();
						field->setChangedPrivate(MANAGER_CHANGE_PARTIAL_RESULT(Computed_field));
					}
				}
			}
		}
		return field->manager_change_status;
	}
	return MANAGER_CHANGE_NONE(Computed_field);
}

Computed_field_element_group *Computed_field_element_group::getConditionalElementGroup(
	cmzn_field *conditionalField, bool &isEmptyGroup) const
{
	isEmptyGroup = false;
	Computed_field_element_group *otherElementGroup =
		dynamic_cast<Computed_field_element_group *>(conditionalField->core);
	if (!otherElementGroup)
	{
		Computed_field_group *group = dynamic_cast<Computed_field_group *>(conditionalField->core);
		if (group)
		{
			otherElementGroup = group->getElementGroupPrivate(this->fe_mesh);
			if (!otherElementGroup)
				isEmptyGroup = true;
		}
	}
	if (otherElementGroup && (otherElementGroup->getSize() == 0))
		isEmptyGroup = true;
	return otherElementGroup;
}

Computed_field_node_group *Computed_field_node_group::create(FE_nodeset *fe_nodeset_in)
{
	Computed_field_node_group *node_group = 0;
	if (fe_nodeset_in)
	{
		DsLabelsGroup *labelsGroup = fe_nodeset_in->createLabelsGroup();
		if (labelsGroup)
		{
			node_group = new Computed_field_node_group(fe_nodeset_in, labelsGroup);
			cmzn::Deaccess(labelsGroup);
		}
	}
	return node_group;
}

int Computed_field_node_group::addObject(cmzn_node *object)
{
	if (!isNodeCompatible(object))
		return CMZN_ERROR_ARGUMENT;
	int return_code = this->labelsGroup->setIndex(object->getIndex(), true);
	if (CMZN_OK == return_code)
	{
		this->invalidateIterators();
		change_detail.changeAdd();
		update();
	}
	return return_code;
}

// GRC future: identical to Computed_field_element_group::addObjectsInIdentifierRange -> move to common base class
int Computed_field_node_group::addObjectsInIdentifierRange(DsLabelIdentifier first, DsLabelIdentifier last)
{
	DsLabels *labels = this->labelsGroup->getLabels();
	if ((first > last) || (!labels))
		return CMZN_ERROR_ARGUMENT;
	int return_code = CMZN_OK;
	DsLabelIndex index;
	DsLabelIdentifier identifier;
	const DsLabelIndex indexSize = labels->getIndexSize();
	if (((last - first) > indexSize) ||
		((!labels->isContiguous()) && ((last - first) > (indexSize/10))))
	{
		for (index = 0; index < indexSize; ++index)
		{
			identifier = labels->getIdentifier(index);
			// invalid identifier == deleted element; skip
			if ((DS_LABEL_IDENTIFIER_INVALID != identifier) &&
				(identifier >= first) && (identifier <= last))
			{
				const int result = this->labelsGroup->setIndex(index, true);
				if ((result != CMZN_OK) && (result != CMZN_ERROR_ALREADY_EXISTS))
				{
					return_code = result;
					break;
				}
			}
		}
	}
	else
	{
		for (identifier = first; identifier <= last; ++identifier)
		{
			index = labels->findLabelByIdentifier(identifier);
			if (DS_LABEL_INDEX_INVALID != index)
			{
				const int result = this->labelsGroup->setIndex(index, true);
				if ((result != CMZN_OK) && (result != CMZN_ERROR_ALREADY_EXISTS))
				{
					return_code = result;
					break;
				}
			}
		}
	}
	return return_code;
}

int Computed_field_node_group::addNodesConditional(cmzn_field_id conditional_field)
{
	if ((!conditional_field) || (conditional_field->manager != this->field->manager))
		return CMZN_ERROR_ARGUMENT;
	bool isEmptyGroup;
	Computed_field_node_group *otherNodeGroup =
		this->getConditionalNodeGroup(conditional_field, isEmptyGroup);
	if (isEmptyGroup)
		return CMZN_OK;
	int return_code = CMZN_OK;
	cmzn_fieldcache *cache = 0;
	const int oldSize = this->getSize();
	cmzn_nodeiterator *iter;
	if (otherNodeGroup)
		iter = otherNodeGroup->createNodeiterator();
	else
	{
		iter = this->fe_nodeset->createNodeiterator();
		cache = new cmzn_fieldcache(FE_region_get_cmzn_region(this->fe_nodeset->get_FE_region()));
		if (!cache)
			return_code = CMZN_ERROR_MEMORY;
	}
	if (!iter)
		return_code = CMZN_ERROR_MEMORY;
	cmzn_node_id node = 0;
	while ((CMZN_OK == return_code) && (0 != (node = cmzn_nodeiterator_next_non_access(iter))))
	{
		if (cache)
		{
			cache->setNode(node);
			if (!cmzn_field_evaluate_boolean(conditional_field, cache))
				continue;
		}
		const int result = this->labelsGroup->setIndex(node->getIndex(), true);
		if ((result != CMZN_OK) && (result != CMZN_ERROR_ALREADY_EXISTS))
		{
			return_code = result;
			break;
		}
	}
	cmzn_nodeiterator_destroy(&iter);
	const int newSize = this->getSize();
	if (newSize != oldSize)
	{
		this->invalidateIterators();
		change_detail.changeAdd();
		update();
	}
	cmzn_fieldcache_destroy(&cache);
	return CMZN_OK;
}

int Computed_field_node_group::removeObject(cmzn_node *object)
{
	if (!isNodeCompatible(object))
		return CMZN_ERROR_ARGUMENT;
	int return_code = this->labelsGroup->setIndex(object->getIndex(), false);
	if (CMZN_OK == return_code)
	{
		this->invalidateIterators();
		change_detail.changeRemove();
		update();
	}
	return return_code;
}

int Computed_field_node_group::removeNodesConditional(cmzn_field_id conditional_field)
{
	if ((!conditional_field) || (conditional_field->manager != this->field->manager))
		return CMZN_ERROR_ARGUMENT;
	bool isEmptyGroup;
	Computed_field_node_group *otherNodeGroup =
		this->getConditionalNodeGroup(conditional_field, isEmptyGroup);
	if (isEmptyGroup)
		return CMZN_OK;
	const int oldSize = this->getSize();
	if (oldSize == 0)
		return CMZN_OK;
	int return_code = CMZN_OK;
	cmzn_fieldcache *cache = 0;
	cmzn_nodeiterator *iter;
	if (otherNodeGroup)
		iter = otherNodeGroup->createNodeiterator(); // could handle this case very efficiently with logical AND of complement
	else
	{
		iter = this->createNodeiterator();
		cache = new cmzn_fieldcache(FE_region_get_cmzn_region(this->fe_nodeset->get_FE_region()));
		if (!cache)
			return_code = CMZN_ERROR_MEMORY;
	}
	if (!iter)
		return_code = CMZN_ERROR_MEMORY;
	if (CMZN_OK == return_code)
	{
		cmzn_node_id node = 0;
		while (0 != (node = iter->nextNode()))
		{
			if (cache)
			{
				cache->setNode(node);
				if (!cmzn_field_evaluate_boolean(conditional_field, cache))
					continue;
			}
			const int result = this->labelsGroup->setIndex(node->getIndex(), false);
			if ((result != CMZN_OK) && (result != CMZN_ERROR_NOT_FOUND))
			{
				return_code = result;
				break;
			}
		}
	}
	cmzn::Deaccess(iter);
	const int newSize = this->getSize();
	if (newSize != oldSize)
	{
		this->invalidateIterators();
		change_detail.changeRemove();
		update();
	}
	return return_code;
}

int Computed_field_node_group::removeNodesInLabelsGroup(DsLabelsGroup &removeNodeLabelsGroup)
{
	cmzn_nodeiterator *iter = this->fe_nodeset->createNodeiterator(&removeNodeLabelsGroup);
	if (!iter)
		return CMZN_ERROR_MEMORY;
	int return_code = CMZN_OK;
	const int oldSize = this->getSize();
	cmzn_node_id node = 0;
	while (0 != (node = iter->nextNode()))
	{
		const int result = this->labelsGroup->setIndex(node->getIndex(), false);
		if ((result != CMZN_OK) && (result != CMZN_ERROR_NOT_FOUND))
		{
			return_code = result;
			break;
		}
	}
	cmzn::Deaccess(iter);
	const int newSize = this->getSize();
	if (newSize < oldSize)
	{
		change_detail.changeRemove();
		update();
	}
	return return_code;
}

int Computed_field_node_group::clear()
{
	if (0 < this->getSize())
	{
		this->labelsGroup->clear();
		this->invalidateIterators();
		change_detail.changeRemove();
		update();
	}
	return CMZN_OK;
};

void Computed_field_node_group::write_btree_statistics() const
{
	display_message(INFORMATION_MESSAGE, "Nodes:\n");
	display_message(INFORMATION_MESSAGE, "  %d labels in bool array\n", this->labelsGroup->getSize());
}

int Computed_field_node_group::addElementNodes(cmzn_element_id element)
{
	if (!isParentElementCompatible(element))
		return CMZN_ERROR_ARGUMENT;
	if (this->fe_nodeset->getFieldDomainType() != CMZN_FIELD_DOMAIN_TYPE_NODES)
		return CMZN_ERROR_ARGUMENT;
	const int oldSize = this->getSize();
	int return_code = cmzn_element_add_nodes_to_labels_group(element, *this->labelsGroup);
	const int newSize = this->getSize();
	if (newSize > oldSize)
	{
		change_detail.changeAdd();
		update();
	}
	return return_code;
};

int Computed_field_node_group::removeElementNodes(cmzn_element_id element)
{
	if (!isParentElementCompatible(element))
		return CMZN_ERROR_ARGUMENT;
	if (this->fe_nodeset->getFieldDomainType() != CMZN_FIELD_DOMAIN_TYPE_NODES)
		return CMZN_ERROR_ARGUMENT;
	const int oldSize = this->getSize();
	int return_code = cmzn_element_remove_nodes_from_labels_group(element, *this->labelsGroup);
	const int newSize = this->getSize();
	if (newSize < oldSize)
	{
		change_detail.changeRemove();
		update();
	}
	return return_code;
};

int Computed_field_node_group::evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache)
{
	const Field_location_node *node_location = cache.get_location_node();
	if (node_location)
	{
		RealFieldValueCache &valueCache = RealFieldValueCache::cast(inValueCache);
		cmzn_node_id node = node_location->get_node();
		valueCache.values[0] = this->containsObject(node) ? 1 : 0;
		return 1;
	}
	return 0;
};

/** remove nodes that have been removed from master nodeset */
int Computed_field_node_group::check_dependency()
{
	if (field)
	{
		DsLabelsChangeLog *nodeChangeLog = this->fe_nodeset->getChangeLog();
		if (nodeChangeLog)
		{
			const int changeSummary = nodeChangeLog->getChangeSummary();
			if (changeSummary & DS_LABEL_CHANGE_TYPE_REMOVE)
			{
				const int oldSize = this->labelsGroup->getSize();
				if (0 < oldSize)
				{
					// special handling for zero nodeset size because labels will have been cleared
					// plus more efficient to clear group
					if (0 == this->fe_nodeset->getSize())
					{
						this->labelsGroup->clear();
					}
					else
					{
						DsLabelIndex index = -1; // DS_LABEL_INDEX_INVALID
						while (this->labelsGroup->incrementIndex(index))
						{
							if (this->fe_nodeset->getNodeIdentifier(index) == DS_LABEL_IDENTIFIER_INVALID)
								this->labelsGroup->setIndex(index, false);
						}
					}
					const int newSize = this->labelsGroup->getSize();
					if (newSize != oldSize)
					{
						this->invalidateIterators();
						change_detail.changeRemove();
						field->setChangedPrivate(MANAGER_CHANGE_PARTIAL_RESULT(Computed_field));
					}
				}
			}
		}
		return field->manager_change_status;
	}
	return MANAGER_CHANGE_NONE(Computed_field);
}

Computed_field_node_group *Computed_field_node_group::getConditionalNodeGroup(
	cmzn_field *conditionalField, bool &isEmptyGroup) const
{
	isEmptyGroup = false;
	Computed_field_node_group *otherNodeGroup = dynamic_cast<Computed_field_node_group *>(conditionalField->core);
	if (!otherNodeGroup)
	{
		Computed_field_group *group = dynamic_cast<Computed_field_group *>(conditionalField->core);
		if (group)
		{
			otherNodeGroup = group->getNodeGroupPrivate(this->fe_nodeset->getFieldDomainType());
			if (!otherNodeGroup)
				isEmptyGroup = true;
		}
	}
	if (otherNodeGroup && (otherNodeGroup->getSize() == 0))
		isEmptyGroup = true;
	return otherNodeGroup;
}

cmzn_field_node_group *cmzn_field_cast_node_group(cmzn_field_id field)
{
	if (field && dynamic_cast<Computed_field_node_group *>(field->core))
	{
		cmzn_field_access(field);
		return (reinterpret_cast<cmzn_field_node_group_id>(field));
	}
	else
	{
		return (NULL);
	}
}

int cmzn_field_node_group_destroy(cmzn_field_node_group_id *node_group_address)
{
	return cmzn_field_destroy(reinterpret_cast<cmzn_field_id *>(node_group_address));
}

Computed_field *cmzn_fieldmodule_create_field_node_group(cmzn_fieldmodule_id field_module, cmzn_nodeset_id nodeset)
{
	Computed_field *field;

	ENTER(cmzn_fieldmodule_create_field_node_group);
	field = (Computed_field *)NULL;
	if (field_module && nodeset && (cmzn_nodeset_get_region_internal(nodeset) ==
		cmzn_fieldmodule_get_region_internal(field_module)))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/false, 1,
			/*number_of_source_fields*/0, NULL,
			/*number_of_source_values*/0, NULL,
			Computed_field_node_group::create(cmzn_nodeset_get_FE_nodeset_internal(nodeset)));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_group.  Invalid argument(s)");
	}
	LEAVE;

	return (field);
} /* cmzn_fieldmodule_create_field_group */

cmzn_field_element_group *cmzn_field_cast_element_group(cmzn_field_id field)
{
	if (field && dynamic_cast<Computed_field_element_group*>(field->core))
	{
		cmzn_field_access(field);
		return (reinterpret_cast<cmzn_field_element_group_id>(field));
	}
	else
	{
		return (NULL);
	}
}

Computed_field *cmzn_fieldmodule_create_field_element_group(cmzn_fieldmodule_id field_module,
		cmzn_mesh_id mesh)
{
	Computed_field *field = 0;
	if (field_module && mesh && (cmzn_mesh_get_region_internal(mesh) ==
		cmzn_fieldmodule_get_region_internal(field_module)))
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/false, 1,
			/*number_of_source_fields*/0, NULL,
			/*number_of_source_values*/0, NULL,
			Computed_field_element_group::create(cmzn_mesh_get_FE_mesh_internal(mesh)));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_group.  Invalid argument(s)");
	}
	return (field);
}

int cmzn_field_element_group_destroy(cmzn_field_element_group_id *element_group_address)
{
	return cmzn_field_destroy(reinterpret_cast<cmzn_field_id *>(element_group_address));
}

void cmzn_field_node_group_list_btree_statistics(
	cmzn_field_node_group_id node_group)
{
	Computed_field_node_group *node_group_core = Computed_field_node_group_core_cast(node_group);
	if (node_group_core)
	{
		node_group_core->write_btree_statistics();
	}
}

void cmzn_field_element_group_list_btree_statistics(
	cmzn_field_element_group_id element_group)
{
	Computed_field_element_group *element_group_core = Computed_field_element_group_core_cast(element_group);
	if (element_group_core)
	{
		element_group_core->write_btree_statistics();
	}
}

#if defined (USE_OPENCASCADE)

cmzn_field_id cmzn_fieldmodule_create_field_cad_primitive_group_template(cmzn_fieldmodule_id field_module)
{
	Computed_field *field = (struct Computed_field *)NULL;

	if (field_module)
	{
		field = Computed_field_create_generic(field_module,
			/*check_source_field_regions*/false, 1,
			/*number_of_source_fields*/0, NULL,
			/*number_of_source_values*/0, NULL,
			new Computed_field_sub_group_object<cmzn_cad_identifier_id>());
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_fieldmodule_create_field_group.  Invalid argument(s)");
	}

	return (field);
}

int cmzn_field_cad_primitive_group_template_destroy(cmzn_field_cad_primitive_group_template_id *cad_primitive_group_address)
{
	return cmzn_field_destroy(reinterpret_cast<cmzn_field_id *>(cad_primitive_group_address));
}

cmzn_field_cad_primitive_group_template_id cmzn_field_cast_cad_primitive_group_template(cmzn_field_id field)
{
	if (dynamic_cast<Computed_field_sub_group_object<cmzn_cad_identifier_id>*>(field->core))
	{
		cmzn_field_access(field);
		return (reinterpret_cast<cmzn_field_cad_primitive_group_template_id>(field));
	}
	else
	{
		return (NULL);
	}
}

int cmzn_field_cad_primitive_group_template_add_cad_primitive(cmzn_field_cad_primitive_group_template_id cad_primitive_group,
	cmzn_cad_identifier_id cad_primitive)
{
	int return_code = 0;
	if (cad_primitive_group && cad_primitive)
	{
		Computed_field_sub_group_object<cmzn_cad_identifier_id> *group_core =
			Computed_field_sub_group_object_core_cast<cmzn_cad_identifier_id,
			cmzn_field_cad_primitive_group_template_id>(cad_primitive_group);
		int identifier = cad_primitive->identifier.number;
		cmzn_cad_identifier_id cad_primitive_copy = new cmzn_cad_identifier(cad_primitive->cad_topology, cad_primitive->identifier);
		//DEBUG_PRINT("=== Adding cad primitive object %p %d %d\n", cad_primitive_copy, cad_primitive->identifier.type, cad_primitive->identifier.number);
		group_core->add_object(identifier, cad_primitive_copy);
		return_code = 1;
	}

	return return_code;
}

int cmzn_field_cad_primitive_group_template_remove_cad_primitive(cmzn_field_cad_primitive_group_template_id cad_primitive_group,
	cmzn_cad_identifier_id cad_primitive)
{
	int return_code = 0;
	if (cad_primitive_group && cad_primitive)
	{
		Computed_field_sub_group_object<cmzn_cad_identifier_id> *group_core =
			Computed_field_sub_group_object_core_cast<cmzn_cad_identifier_id,
				cmzn_field_cad_primitive_group_template_id>(cad_primitive_group);
		int identifier = cad_primitive->identifier.number;
		//DEBUG_PRINT("=== Removing cad primitive object %p %d %d\n", cad_primitive, cad_primitive->identifier.type, cad_primitive->identifier.number);
		cmzn_cad_identifier_id cad_identifier = group_core->get_object(identifier);
		if (cad_identifier)
		{
			//DEBUG_PRINT("Deleting %p\n", cad_identifier);
			delete cad_identifier;
			group_core->remove_object(identifier);
			return_code = 1;
		}
	}

	return return_code;

}

int cmzn_field_cad_primitive_group_template_clear(cmzn_field_cad_primitive_group_template_id cad_primitive_group)
{
	int return_code = 0;

	if (cad_primitive_group)
	{
		Computed_field_sub_group_object<cmzn_cad_identifier_id> *group_core =
			Computed_field_sub_group_object_core_cast<cmzn_cad_identifier_id,
				cmzn_field_cad_primitive_group_template_id>(cad_primitive_group);
		cmzn_cad_identifier_id cad_identifier = group_core->getFirstObject();
		while (cad_identifier != NULL)
		{
			delete cad_identifier;
			cad_identifier = group_core->getNextObject();
		}
		return_code = group_core->clear();
	}

	return return_code;
}

int cmzn_field_cad_primitive_group_template_is_cad_primitive_selected(
	cmzn_field_cad_primitive_group_template_id cad_primitive_group, cmzn_cad_identifier_id cad_primitive)
{
	int return_code = 0;
	if (cad_primitive_group && cad_primitive)
	{
		Computed_field_sub_group_object<cmzn_cad_identifier_id> *group_core =
			Computed_field_sub_group_object_core_cast<cmzn_cad_identifier_id,
			cmzn_field_cad_primitive_group_template_id>(cad_primitive_group);
		int identifier = cad_primitive->identifier.number;
		return_code = group_core->get_object_selected(identifier, cad_primitive);
	}

	return return_code;
}

cmzn_cad_identifier_id cmzn_field_cad_primitive_group_template_get_first_cad_primitive(
	cmzn_field_cad_primitive_group_template_id cad_primitive_group)
{
	cmzn_cad_identifier_id cad_identifier = NULL;
	if (cad_primitive_group)
	{
		Computed_field_sub_group_object<cmzn_cad_identifier_id> *group_core =
			Computed_field_sub_group_object_core_cast<cmzn_cad_identifier_id,
			cmzn_field_cad_primitive_group_template_id>(cad_primitive_group);
		cmzn_cad_identifier_id cad_identifier_from_group = group_core->getFirstObject();
		if (cad_identifier_from_group)
		{
			cad_identifier = new cmzn_cad_identifier(*cad_identifier_from_group);
			//cmzn_field_cad_topology_access(cad_identifier->cad_topology);
		}
	}

	return cad_identifier;
}

cmzn_cad_identifier_id cmzn_field_cad_primitive_group_template_get_next_cad_primitive(
	cmzn_field_cad_primitive_group_template_id cad_primitive_group)
{
	cmzn_cad_identifier_id cad_identifier = NULL;
	if (cad_primitive_group)
	{
		Computed_field_sub_group_object<cmzn_cad_identifier_id> *group_core =
			Computed_field_sub_group_object_core_cast<cmzn_cad_identifier_id,
			cmzn_field_cad_primitive_group_template_id>(cad_primitive_group);
		cmzn_cad_identifier_id cad_identifier_from_group = group_core->getNextObject();
		if (cad_identifier_from_group)
		{
			cad_identifier = new cmzn_cad_identifier(*cad_identifier_from_group);
			//cmzn_field_cad_topology_access(cad_identifier->cad_topology);
		}
	}

	return cad_identifier;
}

#endif /* defined (USE_OPENCASCADE) */

