/*******************************************************************************
FILE : finite_element_adjacent_elements.c

LAST MODIFIED : 13 March 2003

DESCRIPTION :
Functions for finding elements adjacent to other ones.  These functions have
been separated out from finite_element.c due to their dependence on 
indexed_multi_range.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "general/debug.h"
#include "general/indexed_multi_range.h"
#include "mesh/cmiss_element_private.hpp"
#include "computed_field/computed_field_finite_element.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_adjacent_elements.h"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_region.h"
#include "general/message.h"


namespace {

	int FE_field_add_unique_mft_to_vector(struct FE_field *field, void *data_void)
	{
		auto data = static_cast<AdjacentElements1d *>(data_void);
		const FE_mesh_field_data *meshFieldData = field->getMeshFieldData(data->getMesh());
		if (meshFieldData)
		{
			const int componentCount = get_FE_field_number_of_components(field);
			for (int c = 0; c < componentCount; ++c)
			{
				const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
				data->addMFT(mft);
			}
		}
		return 1;
	}

}

int FE_field_add_to_list_if_coordinate(FE_field *field, void *feFieldListVoid)
{
	LIST(FE_field) *feFieldList = static_cast<LIST(FE_field) *>(feFieldListVoid);
	if (field && feFieldList)
	{
		if (field->isTypeCoordinate())
		{
			if (!ADD_OBJECT_TO_LIST(FE_field)(field, feFieldList))
				return 0;
		}
		return 1;
	}
	return 0;
}

AdjacentElements1d::AdjacentElements1d(cmzn_mesh_id meshIn, cmzn_field_id fieldIn) :
	mesh(cmzn_mesh_get_FE_mesh_internal(meshIn)),
	nodeset(this->mesh ? this->mesh->getNodeset() : 0),
	node_element_list(0)
{
	struct LIST(FE_field) *feFieldList = Computed_field_get_defining_FE_field_list(fieldIn);
	if (0 == NUMBER_IN_LIST(FE_field)(feFieldList))
	{
		// use any coordinate fields
		FE_region_for_each_FE_field(cmzn_mesh_get_FE_region_internal(meshIn), FE_field_add_to_list_if_coordinate, (void *)feFieldList);
	}
	FOR_EACH_OBJECT_IN_LIST(FE_field)(FE_field_add_unique_mft_to_vector, static_cast<void*>(this), feFieldList);
	DESTROY(LIST(FE_field))(&feFieldList);
	const size_t mftCount = this->mfts.size();
	if ((mftCount < 1) || (this->nodeset == 0))
		return;
	this->node_element_list = CREATE(LIST(Index_multi_range))();
	cmzn_elementiterator_id iter = this->mesh->createElementiterator();
	cmzn_element_id element;
	int return_code = 1;
	while (return_code && (0 != (element = cmzn_elementiterator_next_non_access(iter))))
	{
		int lastEFTIndex = -1;
		for (size_t f = 0; f < mftCount; ++f)
		{
			const FE_mesh_field_template *mft = this->mfts[f];
			const int eftIndex = mft->getElementEFTIndex(element->getIndex());
			if (eftIndex != lastEFTIndex)
			{
				const FE_mesh_element_field_template_data *meshEFTData = this->mesh->getElementfieldtemplateData(eftIndex);
				const int nodeCount = meshEFTData->getElementfieldtemplate()->getNumberOfLocalNodes();
				if (1 < nodeCount)
				{
					const DsLabelIdentifier elementIdentifier = this->mesh->getElementIdentifier(element->getIndex());
					const DsLabelIndex *nodeIndexes = meshEFTData->getElementNodeIndexes(element->getIndex());
					if (nodeIndexes)
					{
						for (int i = 0; i < nodeCount; ++i)
						{
							const DsLabelIdentifier nodeIdentifier = nodeset->getNodeIdentifier(nodeIndexes[i]);
							if (nodeIdentifier >= 0)
							{
								struct Index_multi_range *node_elements =
									FIND_BY_IDENTIFIER_IN_LIST(Index_multi_range, index_number)(nodeIdentifier, this->node_element_list);
								if (!node_elements)
								{
									node_elements = CREATE(Index_multi_range)(nodeIdentifier);
									if (!ADD_OBJECT_TO_LIST(Index_multi_range)(node_elements, this->node_element_list))
										DESTROY(Index_multi_range)(&node_elements);
								}
								if (!((node_elements)
									&& Index_multi_range_add_range(node_elements, elementIdentifier, elementIdentifier)))
								{
									return_code = 0;
									break;
								}
							}
						}
					}
					lastEFTIndex = eftIndex;
				}
			}
		}
	}
	cmzn_elementiterator_destroy(&iter);
	if (return_code == 0)
	{
		DESTROY(LIST(Index_multi_range))(&this->node_element_list);
		this->node_element_list = 0;
	}
}

AdjacentElements1d::~AdjacentElements1d()
{
	DESTROY(LIST(Index_multi_range))(&this->node_element_list);
}

void AdjacentElements1d::addMFT(const FE_mesh_field_template *mft)
{
	for (size_t i = 0; i < this->mfts.size(); ++i)
		if (this->mfts[i] == mft)
			return;
	this->mfts.push_back(mft);
}

DsLabelIndex AdjacentElements1d::getElementNodeIndexOnFace(struct FE_element *element, int face_index)
{
	int lastEFTIndex = -1;
	const size_t mftCount = this->mfts.size();
	for (size_t f = 0; f < mftCount; ++f)
	{
		const FE_mesh_field_template *mft = this->mfts[f];
		const int eftIndex = mft->getElementEFTIndex(element->getIndex());
		if (eftIndex != lastEFTIndex)
		{
			const FE_mesh_element_field_template_data *meshEFTData = this->mesh->getElementfieldtemplateData(eftIndex);
			const FE_element_field_template *eft = meshEFTData->getElementfieldtemplate();
			const int nodeCount = eft->getNumberOfLocalNodes();
			if (1 < nodeCount)
			{
				const DsLabelIndex nodeIndex = (0 == face_index) ?
					meshEFTData->getElementFirstNodeIndex(element->getIndex()) : meshEFTData->getElementLastNodeIndex(element->getIndex());
				if (nodeIndex != DS_LABEL_INDEX_INVALID)
				{
					return nodeIndex;
				}
			}
		}
	}
	return DS_LABEL_INDEX_INVALID;
}

int AdjacentElements1d::getAdjacentElements(struct FE_element *element,
	int face_index, int *number_of_adjacent_elements,
	struct FE_element ***adjacent_elements)
{
	int i, j, number_of_elements,
		number_of_ranges, range_no, return_code, start, stop;
	struct FE_element *adjacent_element;
	struct Index_multi_range *node_elements;

	if (element && ((0 == face_index) || (1 == face_index)))
	{
		return_code = 1;
		const int element_number = get_FE_element_identifier(element);
		DsLabelIndex nodeIndex = this->getElementNodeIndexOnFace(element, face_index);
		if (nodeIndex >= 0)
		{
			DsLabelIdentifier nodeIdentifier = this->nodeset->getNodeIdentifier(nodeIndex);
			if (NULL != (node_elements = FIND_BY_IDENTIFIER_IN_LIST
				(Index_multi_range,index_number)(nodeIdentifier, node_element_list)))
			{
				/* This list includes the element itself so we are safe but probably
					overallocating the array */
				number_of_elements = Index_multi_range_get_total_number_in_ranges(
					node_elements);
				if (ALLOCATE(*adjacent_elements, struct FE_element *,
					number_of_elements))
				{
					i = 0;
					if (0<(number_of_ranges=Index_multi_range_get_number_of_ranges(
						node_elements)))
					{
						for (range_no=0;return_code&&(range_no<number_of_ranges);range_no++)
						{
							return_code = Index_multi_range_get_range(node_elements,range_no,&start,&stop);
							if (return_code)
							{
								for (j = start ; j <= stop ; j++)
								{
									if (j != element_number)
									{
										adjacent_element = this->mesh->findElementByIdentifier(j);
										if (adjacent_element)
										{
											(*adjacent_elements)[i] = adjacent_element;
											i++;
										}
										else
										{
											display_message(ERROR_MESSAGE,
												"adjacent_FE_element_from_nodes.  "
												"Element %d not found in mesh", j);
											return_code = 0;
										}
									}
								}
							}
						}
					}
					*number_of_adjacent_elements = i;
					if (i == 0)
					{
						/* Don't keep the array if there are no elements */
						DEALLOCATE(*adjacent_elements);
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,"adjacent_FE_element_from_nodes.  "
						"Unable to allocate element array");
					return_code = 0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,"adjacent_FE_element_from_nodes.  "
					"No index object found for node %d", nodeIdentifier);
				return_code = 0;
			}
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"adjacent_FE_element_from_nodes.  "
			"Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}
