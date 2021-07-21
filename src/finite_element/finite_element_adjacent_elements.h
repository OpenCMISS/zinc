/*******************************************************************************
FILE : finite_element_adjacent_elements.h

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
#if !defined (FINITE_ELEMENT_ADJACENT_ELEMENTS_H)
#define FINITE_ELEMENT_ADJACENT_ELEMENTS_H

#include "opencmiss/zinc/element.h"
#include "general/indexed_multi_range.h"

class FE_mesh_field_template;

class AdjacentElements1d
{
	FE_mesh *mesh;
	FE_nodeset *nodeset;
	std::vector<const FE_mesh_field_template *> mfts;
	struct LIST(Index_multi_range) *node_element_list;

	AdjacentElements1d(cmzn_mesh_id meshIn, cmzn_field_id fieldIn);

	DsLabelIndex getElementNodeIndexOnFace(struct FE_element *element, int face_index);

public:

	static AdjacentElements1d *create(cmzn_mesh_id meshIn, cmzn_field_id fieldIn)
	{
		auto adjacentElements = new AdjacentElements1d(meshIn, fieldIn);
		if (adjacentElements->node_element_list)
			return adjacentElements;
		delete adjacentElements;
		return 0;
	}

	~AdjacentElements1d();

	const FE_mesh *getMesh() const
	{
		return this->mesh;
	}

	void addMFT(const FE_mesh_field_template *mft);

	/** For a 1D top level element this routine will return the list of 
	  * <adjacent_elements> not including <element> which share the node indicated by
	  * <node_index>.  <adjacent_elements> is ALLOCATED to the 
	  * correct size and should be DEALLOCATED when calls to this function are finished.
	  * Note elements in adjacent_elements array are not accessed.
	  * @param face_index  0 for xi=0, 1 for xi=1. */
	int getAdjacentElements(struct FE_element *element,
		int face_index, int *number_of_adjacent_elements,
		struct FE_element ***adjacent_elements);

};

#endif /* !defined (FINITE_ELEMENT_ADJACENT_ELEMENTS_H) */
