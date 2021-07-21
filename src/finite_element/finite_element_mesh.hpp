/**
 * FILE : finite_element_mesh.hpp
 *
 * Class defining a domain consisting of a set of finite elements.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (FINITE_ELEMENT_MESH_HPP)
#define FINITE_ELEMENT_MESH_HPP

#include "opencmiss/zinc/types/elementid.h"
#include "opencmiss/zinc/status.h"
#include "datastore/labels.hpp"
#include "datastore/labelschangelog.hpp"
#include "datastore/maparray.hpp"
#include "finite_element/element_field_template.hpp"
#include "finite_element/finite_element_constants.hpp"
#include "finite_element/finite_element_shape.hpp"
#include "general/block_array.hpp"
#include "general/list.h"
#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <vector>

struct FE_field;

class FE_mesh;

class FE_mesh_field_template;

class FE_nodeset;

class FieldDerivative;

/**
 * Template for creating a new element in the given FE_mesh, or redefining
 * an existing element by merging into it.
 * Constist of element shape and definition of fields.
 * @see FE_mesh::create_FE_element()
 * @see FE_mesh::merge_FE_element_template()
 */
class FE_element_template : public cmzn::RefCounted
{
	friend class FE_mesh;

public:

	class FE_field_data
	{
		friend class FE_mesh;

		FE_field *field;
		const int componentCount;
		FE_element_field_template **efts; // allocated if defining, NULL if undefining
		FE_mesh_field_template **cacheComponentMeshFieldTemplates; // cached during merge: not accessed, not thread safe

	public:
		FE_field_data(FE_field *fieldIn);

		~FE_field_data();

		/** Define field/component with element field template. Template is not valid
		  * until all components are defined.
		  * @param componentNumber  Component number to define from 0 to count - 1,
		  * or negative for all components. */
		int define(int componentNumber, FE_element_field_template *eft);

		/** @param componentNumber  Component number to get template for, from 0 to count - 1
		  * @return  Non-accessed pointer to element field template. 0 if field is set to undefine or not found. */
		FE_element_field_template *getComponentElementfieldtemplate(int componentNumber) const
		{
			if ((this->efts) && (0 <= componentNumber) && (componentNumber < this->componentCount))
				return this->efts[componentNumber];
			return 0;
		}

		/** @param componentNumber  Component number from 0 to count - 1. Not checked. */
		FE_mesh_field_template *getCacheComponentMeshFieldTemplate(int componentNumber)
		{
			return this->cacheComponentMeshFieldTemplates[componentNumber];
		}

		/** Temporarily cache current field template for field component. Cleared after merge.
		  * @param componentNumber  Component number from 0 to count - 1. Not checked.
		  * @see FE_mesh::mergeFieldsFromElementTemplate */
		void setCacheComponentMeshFieldTemplate(int componentNumber, FE_mesh_field_template *mft)
		{
			this->cacheComponentMeshFieldTemplates[componentNumber] = mft;
		}

		/** @return  Non-accessed field */
		FE_field *getField() const
		{
			return this->field;
		}

		bool isUndefine() const
		{
			return (0 == this->efts);
		}

		/** Mark field to be undefined */
		void undefine();

		/** @return  True if all components defined, or is undefine */
		bool validate() const;

		/** merge all EFTs into mesh, replacing them with any identical already merged EFTs
		  * @return  True on success, otherwise false. */
		bool mergeElementfieldtemplatesIntoMesh(FE_mesh &mesh);

	};

private:

	FE_mesh *mesh; // Note: accessed: prevents mesh from being destroyed
	FE_element_shape *elementShape; // shape to set for new elements, or 0 to merge without setting shape
	int fieldCount;
	FE_field_data **fields;
	bool fieldsValidated;

	/** Caller must ensure element_shape is of same dimension as mesh */
	FE_element_template(FE_mesh *meshIn, FE_element_shape *elementShapeIn = 0);

	/** Creates a template copying from an existing element */
	//FE_element_template(FE_mesh *mesh_in, cmzn_element *element); // GRC implement?

	~FE_element_template();

	FE_element_template(); // not implemented
	FE_element_template(const FE_element_template&); // not implemented

	/** Add field to list of fields, initially set for undefine.
	  * @return  Index of new field, or -1 if failed. */
	int addField(FE_field *field);

	/** @return index of field in template or -1 if not found*/
	int getFieldIndex(FE_field *field) const;

	/** Define field/component with internal element field template.
	  * Template is not valid until all components are defined.
	  * @param componentNumber  Component number to define from 0 to count - 1,
	  * or negative for all components.
	  * @param eft  Internal element field template: must be locked */
	int defineField(FE_field *field, int componentNumber, FE_element_field_template *eft);

public:

	FE_mesh *getMesh() const
	{
		return this->mesh;
	}

	FE_element_shape *getElementShape() const
	{
		return this->elementShape;
	}

	int setElementShape(FE_element_shape *elementShapeIn);

	/** Define field/component with external element field template. The EFT is validated
	  * and locked before use. Template is not valid until all components are defined.
	  * @param componentNumber  Component number to define from 0 to count - 1,
	  * or negative for all components.
	  * @param eftExt  External element field template */
	int defineField(FE_field *field, int componentNumber, cmzn_elementfieldtemplate *eftExt);

	/** @param componentNumber  Component number to get template for, from 0 to count - 1
	  * @return  Non-accessed pointer to element field template. 0 if field is set to undefine or not found. */
	FE_element_field_template *getElementfieldtemplate(FE_field *field, int componentNumber) const;

	/** Remove field from list so neither defining nor undefining it */
	int removeField(FE_field *field);

	/** Mark field for undefining */
	int undefineField(FE_field *field);

	/** Checks all field definitions are valid, and if so registers all efts with the mesh
	  * @return  True if validated, false if not. */
	bool validate();

	/** @return  True if validate() has passed, false if not */
	bool isValidated() const
	{
		return this->fieldsValidated;
	}
};

/**
 * External handle to a finite element, a region of a mesh charted by a single
 * local 'xi' coordinate system with bounds given by its shape.
 * Note the shape is now held by the owning FE_mesh at the index.
 */
struct cmzn_element
{
	friend class FE_mesh;
private:
	// non-accessed pointer to owning mesh; cleared if mesh is destroyed or element orphaned from mesh
	FE_mesh *mesh;
	// index into mesh labels, maps to unique identifier
	DsLabelIndex index;
	// the number of references held to this element; destroyed once reduces to 0
	int access_count;

	cmzn_element(FE_mesh *meshIn, DsLabelIndex indexIn) :
		mesh(meshIn),
		index(indexIn),
		access_count(1)
	{
	}

	~cmzn_element();

	/** safely disconnect from this mesh. Nodes, scale factors, field DOFs are freed elsewhere when underlying element destroyed */
	void invalidate()
	{
		this->mesh = 0;
		this->index = DS_LABEL_INDEX_INVALID;
	}

public:

	inline cmzn_element *access()
	{
		++(this->access_count);
		return this;
	}

	static inline int deaccess(cmzn_element* &element)
	{
		if (element)
		{
			if (--(element->access_count) == 0)
				delete element;
			element = nullptr;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	static inline void reaccess(cmzn_element* &element, cmzn_element *newElement)
	{
		if (newElement)
			++(newElement->access_count);
		if ((element) && (--(element->access_count) == 0))
			delete element;
		element = newElement;
	}

	inline int getAccessCount() const
	{
		return this->access_count;
	}

	inline DsLabelIdentifier getIdentifier() const;

	inline 	int setIdentifier(int identifier);

	inline DsLabelIndex getIndex() const
	{
		return this->index;
	}

	inline int getDimension() const;

	inline FE_element_shape *getElementShape() const;

	/** @return  Non-accessed mesh, or nullptr if invalid. */
	inline FE_mesh *getMesh() const
	{
		return this->mesh;
	}

	/**
	 * Get the first ancestor of element on ancestorMesh and the mapping from
	 * this element's xi coordinates to those of the ancestor in the form:
	 * xi(ancestor) = b + A.xi(element) where b is in the first column of the
	 * matrix and the rest is A. Recursive to handle 1-D to 3-D case.
	 * @param ancestorMesh  The mesh the ancestor must be on.
	 * @param elementToAncestor  If the returned element is different to this
	 * element, returns the matrix above, otherwise nullptr. Must be at least
	 * MAXIMUM_ELEMENT_XI_DIMENSIONS*MAXIMUM_ELEMENT_XI_DIMENSIONS in size.
	 * @return  Ancestor element or nullptr if not found.
	 */
	cmzn_element *getAncestorConversion(FE_mesh *ancestorMesh, FE_value *elementToAncestor);

};

DECLARE_LIST_CONDITIONAL_FUNCTION(cmzn_element);
DECLARE_LIST_ITERATOR_FUNCTION(cmzn_element);

/** Stores FE_element_field_template and its element-varying local-to-global map data */
class FE_mesh_element_field_template_data
{
	friend class FE_mesh;

	FE_element_field_template *eft;
	const int localNodeCount; // cached from eft to guarantee efficiency
	DsMapArrayLabelIndex localToGlobalNodes; // map from element[local node index] -> global nodes index
	const int localScaleFactorCount; // cached from eft to guarantee efficiency
	DsMapArrayLabelIndex localToGlobalScaleFactors; // map from element[local scale factor index] -> global scale factors index
	// for each element, the number of field components using this eft. Clear per-element data when this drops to zero:
	typedef unsigned short MeshfieldtemplateUsageCountType; // internal use only
	block_array<DsLabelIndex, MeshfieldtemplateUsageCountType> meshfieldtemplateUsageCount;

	FE_mesh_element_field_template_data(const FE_mesh_element_field_template_data &source); // not implemented
	FE_mesh_element_field_template_data& operator=(const FE_mesh_element_field_template_data &source); // not implemented

	void clearElementVaryingData(DsLabelIndex elementIndex);
	void clearAllElementVaryingData();

public:

	FE_mesh_element_field_template_data(FE_element_field_template *eftIn, DsLabels *elementLabels) :
		eft(eftIn->access()),
		localNodeCount(eftIn->getNumberOfLocalNodes()),
		localToGlobalNodes(elementLabels, (localNodeCount > 0) ? localNodeCount : 2, DS_LABEL_INDEX_UNALLOCATED, DS_LABEL_INDEX_INVALID),
		localScaleFactorCount(eftIn->getNumberOfLocalScaleFactors()),
		localToGlobalScaleFactors(elementLabels, (localScaleFactorCount > 0) ? localScaleFactorCount : 2, DS_LABEL_INDEX_UNALLOCATED, DS_LABEL_INDEX_INVALID)
	{
	}

	~FE_mesh_element_field_template_data()
	{
		clearAllElementVaryingData();
		FE_element_field_template::deaccess(this->eft);
	}

	FE_element_field_template *getElementfieldtemplate() const
	{
		return this->eft;
	}

	/** @return  Global index of first node used in element, or DS_LABEL_INDEX_INVALID if none.
	  * @param elementIndex  Label index for element.Caller must ensure this is non-negative. */
	DsLabelIndex getElementFirstNodeIndex(DsLabelIndex elementIndex) const;
	
	/** @return  Global index of last node used by element, or DS_LABEL_INDEX_INVALID if none.
	  * @param elementIndex  Label index for element. Caller must ensure this is non-negative. */
	DsLabelIndex getElementLastNodeIndex(DsLabelIndex elementIndex) const;

	/** @return  Pointer to array of global node indexes for element, or 0 if none. NOT to be freed.
	  * @param elementIndex  Label index for element. Caller must ensure this is non-negative. */
	DsLabelIndex *getElementNodeIndexes(DsLabelIndex elementIndex)
	{
		return this->localToGlobalNodes.getArray(elementIndex);
	}

	/** @return  Pointer to array of global node indexes for element, or 0 if none. NOT to be freed.
	  * @param elementIndex  Label index for element. Caller must ensure this is non-negative. */
	const DsLabelIndex *getElementNodeIndexes(DsLabelIndex elementIndex) const
	{
		return this->localToGlobalNodes.getArray(elementIndex);
	}

	/** @return  Pointer to array of global node indexes for element, or 0 if failed. NOT to be freed.
	  * @param elementIndex  Label index for element. Caller must ensure this is non-negative. */
	DsLabelIndex *getOrCreateElementNodeIndexes(DsLabelIndex elementIndex)
	{
		return this->localToGlobalNodes.getOrCreateArray(elementIndex);
	}

	/** Get a local node for the given element.
	  * @param elementIndex  Element index. Not checked.
	  * @param localNodeIndex  Local index of node to get, starting at 0. Not checked.
	  * @return  Index of node in nodeset, or INVALID if none/failed. */
	DsLabelIndex getElementLocalNode(DsLabelIndex elementIndex, int localNodeIndex) const
	{
		if (elementIndex < 0)
			return CMZN_ERROR_ARGUMENT;
		const DsLabelIndex *elementNodeIndexes = this->getElementNodeIndexes(elementIndex);
		if (elementNodeIndexes)
			return elementNodeIndexes[localNodeIndex];
		return DS_LABEL_INDEX_INVALID;
	}

	/** Set a local node for the given element.
	  * @param elementIndex  Element index.
	  * @param localNodeIndex  Local index of node to set, starting at 0. Not checked.
	  * @param nodeIndex  Index of node in mesh's nodeset, or INVALID to clear.
	  * @return  Result OK on success, any other value on failure. */
	int setElementLocalNode(DsLabelIndex elementIndex, int localNodeIndex, DsLabelIndex nodeIndex);

	/** Set all local nodes for the given element. Does not notify of changes.
	  * @param elementIndex  Element index.
	  * @param nodeIndexes  Array of nodeIndexes to set, size equal to localNodeCount for EFT.
	  * @return  Result OK on success, any other value on failure. */
	int setElementLocalNodes(DsLabelIndex elementIndex, const DsLabelIndex *nodeIndexes);

	/** Set all local nodes for the given element, by identifier.
	  * @param elementIndex  Element index. Not checked.
	  * @param nodeIdentifiers  Array of identifiers of nodes to set, size equal to
	  * localNodeCount for EFT. Use DS_LABEL_IDENTIFIER_INVALID to clear any node.
	  * @return  Result OK on success, any other value on failure. */
	int setElementLocalNodesByIdentifier(DsLabelIndex elementIndex, const DsLabelIndex *nodeIndexes);

	/** Get a scale factor for this element field template in the given element.
	  * @param elementIndex  The element to get scale factor for.
	  * @param localScaleFactorIndex  The local index of the scale factor to get,
	  * from 0 to EFT number of local scale factors - 1. Not checked.
	  * @param value  On success, set to the scale factor value.
	  * @return  Result OK on success, any other value on failure. */
	int getElementScaleFactor(DsLabelIndex elementIndex, int localScaleFactorIndex, FE_value& value);

	/** Set a scale factor for this element field template in the given element.
	  * Notifies all fields at this element changed.
	  * @param elementIndex  The element to set scale factor for.
	  * @param localScaleFactorIndex  The local index of the scale factor to set,
	  * from 0 to EFT number of local scale factors - 1. Not checked.
	  * @param value  The scale factor value to set.
	  * @return  Result OK on success, any other value on failure. */
	int setElementScaleFactor(DsLabelIndex elementIndex, int localScaleFactorIndex, FE_value value);

	/** Get all scale factors for this element field template in the given element.
	  * @param elementIndex  The element to get scale factors for.
	  * @param valuesOut  Array to receive scale factors. Must have enough space for correct number in EFT.
	  * @return  Result OK on success, any other value on failure. */
	int getElementScaleFactors(DsLabelIndex elementIndex, FE_value *valuesOut) const;

	/** Get all scale factors for this element field template in the given element,
	  * creating new scale factor indexes with zero values as needed.
	  * @param elementIndex  The element to get scale factors for.
	  * @param valuesOut  Array to receive scale factors. Must have enough space for correct number in EFT.
	  * @return  Result OK on success, any other value on failure. */
	int getOrCreateElementScaleFactors(DsLabelIndex elementIndex, FE_value *valuesOut);

	/** Set all scale factors for this element field template in the given element.
	  * Notifies all fields at this element changed.
	  * @param elementIndex  The element to set scale factors for.
	  * @param valuesIn  Array of scale factors to set. Required to have correct number for EFT.
	  * @return  Result OK on success, any other value on failure. */
	int setElementScaleFactors(DsLabelIndex elementIndex, const FE_value *valuesIn);

	/** Gets array of scale factor indexes for element, if available.
	  * @param elementIndex  Label index for element. Not checked.
	  * @return  Pointer to array of global scale factor indexes for element, or nullptr if failed. NOT to be freed. */
	const DsLabelIndex *getElementScaleFactorIndexes(DsLabelIndex elementIndex) const
	{
		return this->localToGlobalScaleFactors.getArray(elementIndex);
	}

	/** Gets or creates array of scale factor indexes for element. If creating, guarantees
	  * to create full array of valid indexes. Fails if any scale factor
	  * index cannot be determined, releasing all scale factor indexes found up to that point
	  * and freeing the array. Most common reason for failure is that node-based scale factor
	  * is needed and the respective local node has not been set.
	  * Discovered existing scale factors keep their current values. New scale factors are set to 0.
	  * Fails if eft has no local scale factors.
	  * @param result  Result OK on success, ERROR_NOT_FOUND if cannot create due to absent
	  * nodes or other data, otherwise any other error.
	  * @param elementIndex  Label index for element. Not checked.
	  * @return  Pointer to array of global scale factor indexes for element, or nullptr if failed. NOT to be freed. */
	DsLabelIndex *getOrCreateElementScaleFactorIndexes(int &result, DsLabelIndex elementIndex)
	{
		DsLabelIndex *scaleFactorIndexes = this->localToGlobalScaleFactors.getArray(elementIndex);
		if (scaleFactorIndexes)
		{
			result = CMZN_OK;
			return scaleFactorIndexes;
		}
		return this->createElementScaleFactorIndexes(result, elementIndex);
	}

	void incrementMeshfieldtemplateUsageCount(DsLabelIndex elementIndex);

	void decrementMeshfieldtemplateUsageCount(DsLabelIndex elementIndex);

	DsLabelIndex getElementIndexLimit() const;

	DsLabelIndex getElementIndexStart() const;

	bool hasElementVaryingData() const
	{
		return (this->localNodeCount > 0) || (this->localScaleFactorCount > 0);
	}

	bool localToGlobalNodesIsDense() const;

	int getElementLocalToGlobalNodeMapCount() const;

	bool mergeElementVaryingData(const FE_mesh_element_field_template_data& source);

private:

	/** Creates and fills array of scale factor indexes for element. Fails if any scale factor
	  * index cannot be determined, releasing all scale factor indexes found up to that point
	  * and freeing the array. Most common reason for failure is that node-based scale factor
	  * is needed and the respective local node has not been set.
	  * Only call if no scale factor indexes array exists.
	  * @param result  Result OK on success, ERROR_NOT_FOUND if cannot create due to absent
	  * nodes or other data, otherwise any other error.
	  * @param elementIndex  Label index for element. Checked to be non-negative.
	  * @return  Pointer new array of global scale factor indexes for element, or 0 if failed. NOT to be freed. */
	DsLabelIndex *createElementScaleFactorIndexes(int &result, DsLabelIndex elementIndex);

	/**  Releases usage counts and maps for element scale factor indexes, and removes array.
	  * @param elementIndex  Label index for element. Checked to be non-negative. */
	void destroyElementScaleFactorIndexes(DsLabelIndex elementIndex);

	/** Update scale factor indexes to be for the newNodeIndexes. Only call if node
	  * based scale factors are in use, and node indexes have changed from 
	  * previously valid indexes.
	  * This function is atomic: nothing is changed if it fails.
	  * @return  True on success, false if failed. */
	bool updateElementNodeScaleFactorIndexes(DsLabelIndex elementIndex,
		const DsLabelIndex *newNodeIndexes);

};


/** Stores the mapping of element to element field template and data suitable for
  * defining a scalar field or field component over elements of the mesh.
  * Undefined or -1 value for the eft data map means the field is not defined */
class FE_mesh_field_template
{
public:
	friend class FE_mesh;

	// short integer to save memory in FE_mesh_field_template. Signed to store unset value -1
	typedef short EFTIndexType;
	static const EFTIndexType ELEMENT_FIELD_TEMPLATE_DATA_INDEX_INVALID = -1;

private:
	FE_mesh *mesh; // owning mesh: not ACCESSed since these objects are owned by the mesh
	block_array<DsLabelIndex, EFTIndexType> eftDataMap;
	int mapSize; // number of valid entries in map, for trivial comparisons
	int access_count; // maintained as number of field components using this template

	FE_mesh_field_template(FE_mesh *meshIn) :
		mesh(meshIn),
		eftDataMap(/*blockLengthIn*/CMZN_BLOCK_ARRAY_DEFAULT_BLOCK_SIZE_BYTES/sizeof(EFTIndexType),
			/*allocInitValueIn*/ELEMENT_FIELD_TEMPLATE_DATA_INDEX_INVALID),
		mapSize(0),
		access_count(1)
	{
	}

	FE_mesh_field_template(const FE_mesh_field_template &source);

	~FE_mesh_field_template();

	FE_mesh_field_template(); //  not implemented
	FE_mesh_field_template& operator=(const FE_mesh_field_template &source); // not implemented
public:

	FE_mesh_field_template *access()
	{
		++access_count;
		return this;
	}

	static int deaccess(FE_mesh_field_template* &mft)
	{
		if (!mft)
			return CMZN_ERROR_ARGUMENT;
		--(mft->access_count);
		if (mft->access_count <= 0)
			delete mft;
		mft = 0;
		return CMZN_OK;
	}

	/** @return  Non-accessed mesh owning this mesh field template. */
	FE_mesh *getMesh() const
	{
		return this->mesh;
	}

	/** @return  Number of field components using this mesh field template PLUS external accesses held */
	int getUsageCount() const
	{
		return this->access_count;
	}

	/** @return  True if no element field templates have been set */
	bool isBlank() const
	{
		return (this->mapSize == 0);
	}

	DsLabelIndex getElementIndexLimit() const;

	DsLabelIndex getElementIndexStart() const;

	// Actually just equals access count
	int getFieldComponentUsageCount() const
	{
		return this->access_count;
	}

	inline EFTIndexType getElementEFTIndex(DsLabelIndex elementIndex) const
	{
		return this->eftDataMap.getValue(elementIndex);
	}

	inline FE_element_field_template *getElementfieldtemplate(DsLabelIndex elementIndex) const;

	/** @param elementIndex  Not checked; assume valid
	  * @return  True if template uses eft at element index */
	inline bool hasElementfieldtemplate(DsLabelIndex elementIndex, FE_element_field_template *eft) const
	{
		return this->eftDataMap.hasValue(elementIndex,
			static_cast<EFTIndexType>(eft->getIndexInMesh()));
	}

	bool setElementfieldtemplateIndex(DsLabelIndex elementIndex, EFTIndexType eftIndex);

	/**
	 * Variant taking EFT object. Handles field usage counts.
	 * @param elementIndex  Not checked; assume valid
	 * @param eftIn  Pre-merged element field template for this template's mesh, or 0 to clear.
	 * @return  True on success, false on failure.
	 */
	bool setElementfieldtemplate(DsLabelIndex elementIndex, FE_element_field_template *eftIn)
	{
		return this->setElementfieldtemplateIndex(elementIndex,
			(eftIn) ? eftIn->getIndexInMesh() : ELEMENT_FIELD_TEMPLATE_DATA_INDEX_INVALID);
	}

	bool matchesWithEFTIndexMap(const FE_mesh_field_template &source,
		const std::vector<EFTIndexType> &sourceEFTIndexMap, bool superset) const;

	bool mergeWithEFTIndexMap(const FE_mesh_field_template &source,
		const std::vector<EFTIndexType> &sourceEFTIndexMap);

	bool usesNonLinearBasis() const;

	int getElementsDefinedCount() const;

};

/** Stores reverse map from element index to arrays of embedded node indexes
 * for a given field (of element_xi value type) and nodeset.
 * These objects are held by the mesh while the FE_field holds a handle to it.
 * The FE_field which must call FE_mesh::removeEmbeddedNodeField to free it */
class FE_mesh_embedded_node_field
{
	friend class FE_mesh;

	FE_field *field;  // non-accessed field giving locations in the owning mesh
	FE_nodeset *nodeset;  // not accessed nodes or elements
	// map from element to allocated array of node indexes
	// strictly growning except freed when down to zero size
	// first 2 values are size, allocated size
	dynarray_block_array<DsLabelIndex, DsLabelIndex*> map;

	const int growStep = 16;  // step size of node arrays growth

	FE_mesh_embedded_node_field(FE_field *fieldIn, FE_nodeset *nodesetIn) :
		field(fieldIn),
		nodeset(nodesetIn)
	{
	}

public:

	/** add nodeIndex to the list of indexes for elementIndex. *
	 * Client must guarantee nodeIndex is valid and not already in array. */
	void addNode(DsLabelIndex elementIndex, DsLabelIndex nodeIndex);

	/** remove nodeIndex from list of indexes for elementIndex. *
	 * Client must guarantee nodeIndex is valid and not already in array. */
	void removeNode(DsLabelIndex elementIndex, DsLabelIndex nodeIndex);

	/** Get the last node index for element, which is the most efficient to remove.
	 * @return  Index of last node in map for element, or -1 if none */
	DsLabelIndex getLastNodeIndex(DsLabelIndex elementIndex) const
	{
		DsLabelIndex **nodeIndexesAddress = this->map.getAddress(elementIndex);
		if (!nodeIndexesAddress)
			return -1;
		DsLabelIndex *nodeIndexes = *nodeIndexesAddress;
		if (!nodeIndexes)
			return -1;
		// following requires removeNode to free array when last node removed
		const DsLabelIndex size = nodeIndexes[0];
		return nodeIndexes[size + 1];
	}

	/** Get pointer to node indexes stored for elementIndex, and size.
	 * @param size  On return, size is set to the number of indexes.
	 * @return  Pointer to internal array of node indexes, or nullptr if none. Do not
	 * keep beyond function or through modifications as it may be reallocated. */
	const DsLabelIndex *getNodeIndexes(DsLabelIndex elementIndex, int& size)
	{
		if (elementIndex >= 0)
		{
			DsLabelIndex **nodeIndexesAddress = this->map.getAddress(elementIndex);
			if (nodeIndexesAddress)
			{
				DsLabelIndex *nodeIndexes = *nodeIndexesAddress;
				if (nodeIndexes)
				{
					size = nodeIndexes[0];
					return nodeIndexes + 2;
				}
			}
		}
		size = 0;
		return nullptr;
	}

};

/**
 * A set of elements in the FE_region.
 */
class FE_mesh
{
public:

	/** Stores element shape and face maps for that shape */
	class ElementShapeFaces
	{
		FE_element_shape *shape;
		const cmzn_element_shape_type shape_type;
		const int faceCount;
		DsMapArrayLabelIndex faces; // map to connected element indexes from face mesh

	public:
		ElementShapeFaces(DsLabels *labels, FE_element_shape *shapeIn) :
			shape(ACCESS(FE_element_shape)(shapeIn)),
			shape_type(FE_element_shape_get_simple_type(shapeIn)),
			faceCount(FE_element_shape_get_number_of_faces(shapeIn)),
			faces(labels, faceCount > 0 ? faceCount : 2, DS_LABEL_INDEX_UNALLOCATED, DS_LABEL_INDEX_INVALID)
		{
		}

		~ElementShapeFaces()
		{
			DEACCESS(FE_element_shape)(&this->shape);
		}

		FE_element_shape *getElementShape() const
		{
			return this->shape;
		}

		cmzn_element_shape_type getElementShapeType() const
		{
			return this->shape_type;
		}

		int getFaceCount() const
		{
			return this->faceCount;
		}

		/** return face number for face type, or negative if invalid */
		int faceTypeToNumber(cmzn_element_face_type faceType)
		{
			// temporary, until other element shape face type enums and mappings to face numbers are supported:
			int faceNumber = static_cast<int>(faceType) - CMZN_ELEMENT_FACE_TYPE_XI1_0;
			if (faceNumber < this->faceCount)
				return faceNumber;
			return -1;
		}

		/** clear face element indexes held for elementIndex */
		void destroyElementFaces(DsLabelIndex elementIndex)
		{
			this->faces.destroyArray(elementIndex);
		}

		/** @return  Array of faceCount face element indexes, or 0 if not allocated. NOT to be freed. */
		DsLabelIndex *getElementFaces(DsLabelIndex elementIndex)
		{
			return this->faces.getArray(elementIndex);
		}

		/** @return  Array of faceCount face element indexes, or 0 if not allocated. NOT to be freed. */
		const DsLabelIndex *getElementFaces(DsLabelIndex elementIndex) const
		{
			return this->faces.getArray(elementIndex);
		}

		/** @return  Existing or new array of faceCount face element indexes, or 0 if failed. NOT to be freed.
		 * New array contents are initialised to DS_LABEL_INDEX_INVALID. */
		DsLabelIndex *getOrCreateElementFaces(DsLabelIndex elementIndex)
		{
			return this->faces.getOrCreateArray(elementIndex);
		}

		/** convenient function for getting a single face for an element */
		DsLabelIndex getElementFace(DsLabelIndex elementIndex, int faceNumber);

	};

private:

	FE_region *fe_region; // not accessed
	const int dimension;

	DsLabels labels; // element identifiers

	// element shape and face/parent mappings
	int elementShapeFacesCount;
	ElementShapeFaces **elementShapeFacesArray;
	typedef unsigned char ElementShapeMapIndexType;
	block_array<DsLabelIndex, ElementShapeMapIndexType> elementShapeMap; // map element index -> shape faces index, if not all elements have same shape
	block_array<DsLabelIndex,DsLabelIndex*> parents; // map to number of parents followed by parent element indexes in order

	// map element index -> element object (accessed)
	block_array<DsLabelIndex, cmzn_element*> fe_elements;

	// maintain list of EFTs for this mesh, whether merged in elementFieldTemplateData
	// or externally held, so can clear their mesh pointer on mesh destruction
	std::set<FE_element_field_template *> allElementfieldtemplates;

	// array of element field templates with common element mapping data
	// mesh field templates store indexes into this array
	int elementFieldTemplateDataCount;
	FE_mesh_element_field_template_data **elementFieldTemplateData;

	std::list<FE_mesh_field_template*> meshFieldTemplates;

	// vector of reverse maps from elements to arrays of node indexes held
	// for all field of element_xi type for nodeset with nodes embedded in this mesh
	// indexes are given out for lifetime of nodeset x field
	std::list<FE_mesh_embedded_node_field*> embeddedNodeFields;

	DsLabelIndex scaleFactorsCount;  // actual number of scale factors. Note can be holes in scaleFactors array
	DsLabelIndex scaleFactorsIndexSize;  // allocated size of scale factors array, including holes
	block_array<DsLabelIndex, FE_value> scaleFactors;
	block_array<DsLabelIndex, int> scaleFactorUsageCounts;  // number of element x efts using this scale factor
	std::map<int, DsLabelIndex> globalScaleFactorsIndex;  // index for global scale factor type
	// zero terminated allocated array alternate identifier, index pairs. PATCH type uses negative identifier, GENERAL positive
	dynarray_block_array<DsLabelIndex, DsLabelIndex*> nodeScaleFactorsIndex;

	// cache last element template merged in, so not expensively recording field changes
	// must clear when element template changed or deleted, and in extractChangeLog()
	// not accessed, since FE_element_template accesses mesh
	FE_element_template *lastMergedElementTemplate;

	// following are cleared when detached from owning FE_region (i.e. when it is destroyed)
	FE_mesh *parentMesh; // not accessed
	FE_mesh *faceMesh; // not accessed
	FE_nodeset *nodeset; // not accessed

	/* log of elements added, removed or otherwise changed */
	DsLabelsChangeLog *changeLog;

	/* information for defining faces */
	/* existence of element_type_node_sequence_list can tell us whether faces
		 are being defined */
	struct LIST(FE_element_type_node_sequence) *element_type_node_sequence_list;
	bool definingFaces;

	FieldDerivative *fieldDerivatives[MAXIMUM_MESH_DERIVATIVE_ORDER];

	// list of element iterators to invalidate when mesh destroyed
	cmzn_elementiterator *activeElementIterators;

	mutable int access_count;

private:

	FE_mesh(FE_region *fe_regionIn, int dimensionIn);

	~FE_mesh();

	void createChangeLog();

	int findOrCreateFace(DsLabelIndex parentIndex, int faceNumber, DsLabelIndex& faceIndex);

	int removeElementPrivate(DsLabelIndex elementIndex);

	/** @param  elementIndex  Valid element index >= 0. Not checked.
	  * @return  Index of ElementShapeFaces for element in mesh, starting at 0.
	  * Note this returns 0 where there are no shapes. */
	inline int getElementShapeFacesIndexPrivate(DsLabelIndex elementIndex) const
	{
		if (this->elementShapeFacesCount > 1)
			return static_cast<int>(this->elementShapeMap.getValue(elementIndex));
		return 0;
	}

	int addElementParent(DsLabelIndex elementIndex, DsLabelIndex parentIndex);

	int removeElementParent(DsLabelIndex elementIndex, DsLabelIndex parentIndex);

	void clearElementParents(DsLabelIndex elementIndex);

	void clearElementFaces(DsLabelIndex elementIndex);

	ElementShapeFaces *setElementShape(DsLabelIndex elementIndex, FE_element_shape *element_shape);

	bool setElementShapeFromElementTemplate(DsLabelIndex elementIndex, FE_element_template *element_template);

	bool mergeFieldsFromElementTemplate(DsLabelIndex elementIndex, FE_element_template *elementTemplate);

	void destroyElementPrivate(DsLabelIndex elementIndex);

	DsLabelIndex createElement(DsLabelIdentifier identifier);

	cmzn_element *createElementObject(DsLabelIdentifier identifier);

	void clearElementFieldData();

	/** Add a new scale factor index and give it usage count 1.
	  * Caller is required to set up node or global identifier mapping, and set value.
	  * @return  New index, or DS_LABEL_INDEX_INVALID if failed */
	DsLabelIndex createScaleFactorIndex();

public:

	static FE_mesh *create(FE_region *fe_region, int dimension)
	{
		if (fe_region && (0 < dimension) && (dimension <= MAXIMUM_ELEMENT_XI_DIMENSIONS))
			return new FE_mesh(fe_region, dimension);
		return 0;
	}

	void detach_from_FE_region();

	void clearChangeLog()
	{
		this->createChangeLog();
	}

	int addElementfieldtemplate(FE_element_field_template *eft);

	int removeElementfieldtemplate(FE_element_field_template *eft);

	/** @param eftIndex  Index from 0 to number in mesh - 1. Not checked.
	  * @return  Non-accessed element field template. */
	FE_element_field_template *getElementfieldtemplateFromIndex(int eftIndex) const
	{
		return this->elementFieldTemplateData[eftIndex]->getElementfieldtemplate();
	}

	/** @return  Number of element field template data. */
	int getElementfieldtemplateDataCount() const
	{
		return this->elementFieldTemplateDataCount;
	}

	/** @param eftIndex  Index of EFT in this mesh. Not checked.
	  * @return  Non-accessed mesh element field template data, or 0 if none. */
	const FE_mesh_element_field_template_data *getElementfieldtemplateData(int eftIndex) const
	{
		return this->elementFieldTemplateData[eftIndex];
	}

	/** @param eftIndex  Index of EFT in this mesh. Not checked.
	  * @return  Non-accessed mesh element field template data, or 0 if none. */
	FE_mesh_element_field_template_data *getElementfieldtemplateData(int eftIndex)
	{
		return this->elementFieldTemplateData[eftIndex];
	}

	/** @param eft  Element field template from this mesh.
	  * @return  Non-accessed mesh element field template data. */
	FE_mesh_element_field_template_data *getElementfieldtemplateData(const FE_element_field_template *eft) const
	{
		if (eft && (eft->getMesh() == this) && (eft->isMergedInMesh()))
			return this->elementFieldTemplateData[eft->getIndexInMesh()];
		return 0;
	}

	FE_element_field_template *findMergedElementfieldtemplate(FE_element_field_template *eftIn);

	FE_element_field_template *mergeElementfieldtemplate(FE_element_field_template *eftIn);

	FE_mesh_field_template *createBlankMeshFieldTemplate();

	FE_mesh_field_template *cloneMeshFieldTemplate(const FE_mesh_field_template *source);

	FE_mesh_field_template *getOrCreateBlankMeshFieldTemplate();

	void removeMeshFieldTemplate(FE_mesh_field_template *meshFieldTemplate);

	/** Adds to mesh FE_mesh_embedded_node_field object for the field and nodeset,
	 * held by the mesh so knows which fields embed nodes in this mesh.
	 * Fields may optionally maintain reverse maps from elements to nodes in this
	 * structure.
	 * @return  FE_mesh_embedded_node_field for field to store. Field must call
	 * removeEmbeddedNodeField to free.
	 */
	FE_mesh_embedded_node_field *addEmbeddedNodeField(FE_field *field, FE_nodeset *nodeset);
	
	/** Remove the embeddedNodeField from mesh and delete.
	 * @param embeddedNodeField  Client pointer to embedded node field. Cleared by this function */
	void removeEmbeddedNodeField(FE_mesh_embedded_node_field*& embeddedNodeField);

	bool equivalentFieldsInElements(DsLabelIndex elementIndex1, DsLabelIndex elementIndex2) const;

	const FE_mesh *access() const
	{
		++(this->access_count);
		return this;
	}

	FE_mesh *access()
	{
		++(this->access_count);
		return this;
	}

	static void deaccess(const FE_mesh *&mesh)
	{
		if (mesh)
		{
			--(mesh->access_count);
			if (mesh->access_count <= 0)
				delete mesh;
			mesh = 0;
		}
	}

	static void deaccess(FE_mesh *&mesh)
	{
		if (mesh)
		{
			--(mesh->access_count);
			if (mesh->access_count <= 0)
				delete mesh;
			mesh = 0;
		}
	}

	void elementChange(DsLabelIndex elementIndex, int change);

	void elementFieldChange(DsLabelIndex elementIndex, int change, FE_field *fe_field);

	void elementAllFieldChange(DsLabelIndex elementIndex, int change);

	int getDimension() const
	{
		return this->dimension;
	}

	FE_region *get_FE_region() const
	{
		return this->fe_region;
	}

	FE_mesh *getFaceMesh() const
	{
		return this->faceMesh;
	}

	void setFaceMesh(FE_mesh *faceMeshIn)
	{
		this->faceMesh = faceMeshIn;
	}

	FE_mesh *getParentMesh() const
	{
		return this->parentMesh;
	}

	void setParentMesh(FE_mesh *parentMeshIn)
	{
		this->parentMesh = parentMeshIn;
	}

	void setNodeset(FE_nodeset *nodesetIn)
	{
		this->nodeset = nodesetIn;
	}

	/** @return  Nodeset for this mesh, or 0 if detached from FE_region. */
	FE_nodeset *getNodeset() const
	{
		return this->nodeset;
	}

	const char *getName() const;

	const DsLabels& getLabels() const
	{
		return this->labels;
	}

	void clear();

	/** @return Accessed changes */
	DsLabelsChangeLog *extractChangeLog();

	/** @retrun non-Accessed changes */
	DsLabelsChangeLog *getChangeLog()
	{
		return this->changeLog;
	}

	int getElementShapeFacesCount() const
	{
		return this->elementShapeFacesCount;
	}

	inline int getElementShapeFacesIndex(DsLabelIndex elementIndex) const
	{
		if (elementIndex >= 0)
			return this->getElementShapeFacesIndexPrivate(elementIndex);
		return -1;
	}

	cmzn_element_shape_type getElementShapeTypeAtIndex(int index) const
	{
		if ((index < 0) || (index >= this->elementShapeFacesCount))
			return CMZN_ELEMENT_SHAPE_TYPE_INVALID;
		return this->elementShapeFacesArray[index]->getElementShapeType();
	}

	/** @param  elementIndex  Valid element index >= 0. Not checked. */
	inline const ElementShapeFaces *getElementShapeFaces(DsLabelIndex elementIndex) const
	{
		const int elementShapeFacesIndex = this->getElementShapeFacesIndexPrivate(elementIndex);
		if (this->elementShapeFacesArray)
			return this->elementShapeFacesArray[elementShapeFacesIndex];
		return 0;
	}

	/** Use with care. Only made public for use with EXReader.
	  * @param  elementIndex  Valid element index >= 0. Not checked. */
	inline ElementShapeFaces *getElementShapeFaces(DsLabelIndex elementIndex)
	{
		const int elementShapeFacesIndex = this->getElementShapeFacesIndexPrivate(elementIndex);
		if (this->elementShapeFacesArray)
			return this->elementShapeFacesArray[elementShapeFacesIndex];
		return 0;
	}

	inline const ElementShapeFaces *getElementShapeFacesConst(DsLabelIndex elementIndex) const
	{
		return const_cast<FE_mesh*>(this)->getElementShapeFaces(elementIndex);
	}

	FE_element_shape *getElementShape(DsLabelIndex elementIndex) const
	{
		const ElementShapeFaces *elementShapeFaces = this->getElementShapeFaces(elementIndex);
		if (elementShapeFaces)
			return elementShapeFaces->getElementShape();
		return 0;
	}

	cmzn_element_shape_type getElementShapeType(DsLabelIndex elementIndex) const
	{
		const ElementShapeFaces *elementShapeFaces = this->getElementShapeFaces(elementIndex);
		if (elementShapeFaces)
			return elementShapeFaces->getElementShapeType();
		return CMZN_ELEMENT_SHAPE_TYPE_INVALID;
	}

	/** get size i.e. number of elements in mesh */
	int getSize() const
	{
		return this->labels.getSize();
	}

	/** get labels index size, gives index limit for iterating in index order */
	DsLabelIndex getLabelsIndexSize() const
	{
		return this->labels.getIndexSize();
	}

	inline bool hasElement(DsLabelIndex elementIndex) const
	{
		return this->labels.hasIndex(elementIndex);
	}

	inline DsLabelIdentifier getElementIdentifier(DsLabelIndex elementIndex) const
	{
		return this->labels.getIdentifier(elementIndex);
	}

	/** @return  Non-accessed element object at index */
	inline cmzn_element *getElement(DsLabelIndex elementIndex) const
	{
		cmzn_element *element = 0;
		if (elementIndex >= 0)
			this->fe_elements.getValue(elementIndex, element);
		return element;
	}

	DsLabelIdentifier get_next_FE_element_identifier(DsLabelIdentifier start_identifier)
	{
		return this->labels.getFirstFreeIdentifier(start_identifier);
	}

	void list_btree_statistics();

	bool containsElement(cmzn_element *element) const
	{
		return (element) && (element->getMesh() == this) && (element->getIndex() >= 0);
	}

	DsLabelIndex findIndexByIdentifier(DsLabelIdentifier identifier) const
	{
		return this->labels.findLabelByIdentifier(identifier);
	}

	/** @return  Non-accessed element */
	cmzn_element *findElementByIdentifier(DsLabelIdentifier identifier) const
	{
		return this->getElement(this->labels.findLabelByIdentifier(identifier));
	}

	void removeElementiterator(cmzn_elementiterator *iterator); // private, but needed by cmzn_elementiterator

	cmzn_elementiterator *createElementiterator(DsLabelsGroup *labelsGroup = 0);

	/** @return First element in mesh for which function returns true / non-zero.
	 * @param conditional_function  If none, return first element, if any. */
	cmzn_element *get_first_FE_element_that(
		LIST_CONDITIONAL_FUNCTION(cmzn_element) *conditional_function, void *user_data_void);

	int for_each_FE_element(LIST_ITERATOR_FUNCTION(cmzn_element) iterator_function, void *user_data_void);

	DsLabelsGroup *createLabelsGroup();

	int setElementIdentifier(DsLabelIndex elementIndex, int identifier);

	FE_element_template *create_FE_element_template(FE_element_shape *element_shape = 0);

	cmzn_element *create_FE_element(int identifier, FE_element_template *elementTemplate);

	cmzn_element *get_or_create_FE_element_with_identifier(int identifier,
		struct FE_element_shape *element_shape);

	int merge_FE_element_template(cmzn_element *destination, FE_element_template *elementTemplate);

	/** Call if element template is destroyed or modified to ensure mesh is
	  * not caching it for efficient field change notification */
	void noteChangedElementTemplate(FE_element_template *elementTemplate)
	{
		if (elementTemplate == this->lastMergedElementTemplate)
			this->lastMergedElementTemplate = 0;
	}

	DsLabelIndex getElementFace(DsLabelIndex elementIndex, int faceNumber)
	{
		ElementShapeFaces *elementShapeFaces = this->getElementShapeFaces(elementIndex);
		if (!elementShapeFaces)
			return DS_LABEL_INDEX_INVALID;
		return elementShapeFaces->getElementFace(elementIndex, faceNumber);
	}

	int setElementFace(DsLabelIndex elementIndex, int faceNumber, DsLabelIndex faceIndex);

	int getElementFaceNumber(DsLabelIndex elementIndex, DsLabelIndex faceIndex) const;

	/** @return  Number of parent elements.
	  * @param parents  If has parents, pointer to parent element indexes. Not to be freed */
	int getElementParents(DsLabelIndex elementIndex, const DsLabelIndex *&parents) const
	{
		if (elementIndex >= 0)
		{
			DsLabelIndex *parentsArray;
			if (this->parents.getValue(elementIndex, parentsArray) && parentsArray)
			{
				parents = parentsArray + 1;
				return *parentsArray;
			}
		}
		return 0;
	}

	bool elementHasParentInGroup(DsLabelIndex elementIndex, const DsLabelsGroup& parentGroup) const
	{
		const DsLabelIndex *parents;
		const int parentCount = this->getElementParents(elementIndex, parents);
		for (int p = 0; p < parentCount; ++p)
			if (parentGroup.hasIndex(parents[p]))
				return true;
		return false;
	}

	bool elementHasNodeInGroup(DsLabelIndex elementIndex, const DsLabelsGroup& nodeLabelsGroup) const;

	bool addElementNodesToGroup(DsLabelIndex elementIndex, DsLabelsGroup& nodeLabelsGroup);

	bool removeElementNodesFromGroup(DsLabelIndex elementIndex, DsLabelsGroup& nodeLabelsGroup);

	/** @return  Number of parent elements. */
	inline int getElementParentsCount(DsLabelIndex elementIndex) const
	{
		if (this->parentMesh)
		{
			const DsLabelIndex *parents;
			return this->getElementParents(elementIndex, parents);
		}
		return 0;
	}

	// GRC restore implementation from descendant up
	bool isElementAncestor(DsLabelIndex elementIndex, const FE_mesh *descendantMesh, DsLabelIndex descendantIndex);

	/** return true if element or parent has exactly 1 parent element */
	bool isElementExterior(DsLabelIndex elementIndex);

	/** return true if elementIndex has faceIndex on the given face */
	bool isElementFaceOfType(DsLabelIndex elementIndex, DsLabelIndex faceIndex, cmzn_element_face_type faceType)
	{
		ElementShapeFaces *elementShapeFaces = this->getElementShapeFaces(elementIndex);
		if (elementShapeFaces)
		{
			const DsLabelIndex *faces = elementShapeFaces->getElementFaces(elementIndex);
			if (faces)
			{
				const int faceNumber = elementShapeFaces->faceTypeToNumber(faceType);
				if ((0 <= faceNumber) && (faces[faceNumber] == faceIndex))
					return true;
			}
		}
		return false;
	}

	/** return index of parent element that either this element is on the given face of,
		* or the parent is on the given face of a top-level element. */
	DsLabelIndex getElementParentOnFace(DsLabelIndex elementIndex, cmzn_element_face_type faceType);

	DsLabelIndex getElementFirstNeighbour(DsLabelIndex elementIndex, int faceNumber, int &newFaceNumber);

	int defineElementFaces(DsLabelIndex elementIndex);

	int begin_define_faces();

	void end_define_faces();

	int define_faces();

	int destroyElement(cmzn_element *element);

	int destroyAllElements();

	int destroyElementsInGroup(DsLabelsGroup& labelsGroup);

	/** @return  Non-accessed field derivative w.r.t. mesh chart of given order */
	FieldDerivative *getFieldDerivative(int order) const
	{
		if ((order < 1) || (order > MAXIMUM_MESH_DERIVATIVE_ORDER))
			return nullptr;
		return this->fieldDerivatives[order - 1];
	}

	/** Get derivative w.r.t. mesh chart followed by supplied field derivative operation.
	 * @param fieldDerivative  Derivative to apply after. If it involves mesh derivatives,
	 * they must be for this mesh.
	 * @return  Non-accessed field derivative or nullptr on error */
	FieldDerivative *getHigherFieldDerivative(const FieldDerivative& fieldDerivative);

	/** @param scaleFactorIndex  Not checked. Must be valid. */
	FE_value getScaleFactor(DsLabelIndex scaleFactorIndex) const
	{
		return this->scaleFactors.getValue(scaleFactorIndex);
	}

	/** @param scaleFactorIndex  Not checked. Must be valid. */
	int getScaleFactorUsageCount(DsLabelIndex scaleFactorIndex) const
	{
		return this->scaleFactorUsageCounts.getValue(scaleFactorIndex);
	}

	/** Query whether scale factor exists at index.
	  * @param scaleFactorIndex  Index from 0 to scaleFactorsIndexSize - 1. Checked. */
	bool hasScaleFactor(DsLabelIndex scaleFactorIndex) const
	{
		if ((scaleFactorIndex < 0) || (scaleFactorIndex >= this->scaleFactorsIndexSize))
		{
			display_message(ERROR_MESSAGE, "FE_mesh::hasScaleFactor.  Index out of range");
			return false;
		}
		return (this->scaleFactorUsageCounts.getValue(scaleFactorIndex) > 0);
	}

	/** Sets scale factor at index. Does not notify.
	  * @param scaleFactorIndex  Not checked. Must be valid. */
	void setScaleFactor(DsLabelIndex scaleFactorIndex, FE_value value)
	{
		this->scaleFactors.setValue(scaleFactorIndex, value);
	}

	/** Get index for scale factor with type, object index and identifier, and increment its
	  * usage count. If none found, create a new scale factor with usage count 1 and return its index.
	  * @return  Scale factor index, or DS_LABEL_INDEX_INVALID if failed */
	DsLabelIndex getOrCreateScaleFactorIndex(cmzn_elementfieldtemplate_scale_factor_type scaleFactorType,
		DsLabelIndex objectIndex, int scaleFactorIdentifier);

	/** Decrement usage count for scale factor index, and if it drops to zero remove relevant
	  * node or global map to it.
	  * @param scaleFactorIndex  Up to caller to ensure valid. */
	void releaseScaleFactorIndex(DsLabelIndex scaleFactorIndex,
		cmzn_elementfieldtemplate_scale_factor_type scaleFactorType,
		DsLabelIndex objectIndex, int scaleFactorIdentifier);

	bool checkConvertLegacyNodeParameters(FE_nodeset *targetNodeset);

	bool canMerge(const FE_mesh &source) const;

	bool mergePart1Elements(const FE_mesh &source);

	bool mergePart2Fields(const FE_mesh &source);

};

inline DsLabelIdentifier cmzn_element::getIdentifier() const
{
	if (this->mesh)
		return this->mesh->getElementIdentifier(this->index);
	return DS_LABEL_IDENTIFIER_INVALID;
}

inline int cmzn_element::setIdentifier(int identifier)
{
	if (this->mesh)
		return this->mesh->setElementIdentifier(this->index, identifier);
	return CMZN_ERROR_GENERAL;
}

inline int cmzn_element::getDimension() const
{
	if (this->mesh)
		return this->mesh->getDimension();
	display_message(ERROR_MESSAGE, "Element getDimension.  Invalid element");
	return 0;
}

inline FE_element_shape *cmzn_element::getElementShape() const
{
	if (this->mesh)
		return this->mesh->getElementShape(this->index);
	display_message(ERROR_MESSAGE, "Element getElementShape.  Invalid element");
	return 0;
}

/** @param elementIndex  Not checked; assume valid */
inline FE_element_field_template *FE_mesh_field_template::getElementfieldtemplate(DsLabelIndex elementIndex) const
{
	const EFTIndexType eftIndex = this->eftDataMap.getValue(elementIndex);
	if (eftIndex < 0)
		return 0;
	return this->mesh->getElementfieldtemplateFromIndex(eftIndex);
}


struct cmzn_elementiterator : public cmzn::RefCounted
{
	friend class FE_mesh;

private:
	FE_mesh *fe_mesh;
	DsLabelIterator *iter;
	cmzn_elementiterator *nextIterator; // for linked list of active iterators in FE_mesh

	// takes ownership of iter_in access count
	cmzn_elementiterator(FE_mesh *fe_mesh_in, DsLabelIterator *iter_in) :
		fe_mesh(fe_mesh_in),
		iter(iter_in),
		nextIterator(0)
	{
	}

	virtual ~cmzn_elementiterator()
	{
		if (this->fe_mesh)
			this->fe_mesh->removeElementiterator(this);
		cmzn::Deaccess(this->iter);
	}

	void detachFromMesh()
	{
		// sufficient to clear fe_mesh as only called from fe_mesh destructor
		this->fe_mesh = 0;
	}

	template<class REFCOUNTED>
		friend inline void cmzn::Deaccess(REFCOUNTED*& elementIterator);

public:

	/** @return  Non-accessed element or 0 if iteration ended or iterator invalidated */
	cmzn_element *nextElement()
	{
		if (this->fe_mesh)
		{
			const DsLabelIndex elementIndex = this->iter->nextIndex();
			return this->fe_mesh->getElement(elementIndex);
		}
		return 0;
	}

	void setIndex(DsLabelIndex index)
	{
		this->iter->setIndex(index);
	}
};


/** Stores field component definition on a mesh as a FE_mesh_field_template plus
  * per-element parameters. Owned by field. */
class FE_mesh_field_data
{
	friend class FE_mesh;

public:
	class ComponentBase
	{
		friend class FE_mesh;
		friend class FE_mesh_field_data;

		FE_mesh_field_template *meshFieldTemplate; // accessed

	public:

		ComponentBase(FE_mesh_field_template *meshFieldTemplateIn) :
			meshFieldTemplate(meshFieldTemplateIn->access())
		{
		}

		virtual ~ComponentBase()
		{
			FE_mesh_field_template::deaccess(this->meshFieldTemplate);
		}

		virtual void clearElementData(DsLabelIndex elementIndex) = 0;

		/** @return  Non-accessed field template */
		const FE_mesh_field_template *getMeshfieldtemplate() const
		{
			return this->meshFieldTemplate;
		}

		/** @return  Non-accessed field template */
		FE_mesh_field_template *getMeshfieldtemplate()
		{
			return this->meshFieldTemplate;
		}

		/** Currently assumes previous template had no per-element values
		  * @param fieldTemplateIn  Field template: not checked; must be valid for mesh */
		void setMeshfieldtemplate(FE_mesh_field_template *meshFieldTemplateIn)
		{
			meshFieldTemplateIn->access();
			FE_mesh_field_template::deaccess(this->meshFieldTemplate);
			this->meshFieldTemplate = meshFieldTemplateIn;
		}

		virtual bool mergeElementValues(const ComponentBase *sourceBase) = 0;

	};

	template <typename ValueType> class Component : public ComponentBase
	{
		friend class FE_mesh;

		block_array<DsLabelIndex, ValueType> elementScalarDOFs;
		dynarray_block_array<DsLabelIndex, ValueType*> elementVectorDOFs;

	public:

		Component(FE_mesh_field_template *meshFieldTemplateIn) :
			ComponentBase(meshFieldTemplateIn)
		{
		}

		virtual ~Component()
		{
		}

		/** @param elementIndex  Element index >= 0. Not checked. */
		virtual void clearElementData(DsLabelIndex elementIndex)
		{
			ValueType** valuePtrAddress = this->elementVectorDOFs.getAddress(elementIndex);
			if (valuePtrAddress)
			{
				delete[] *valuePtrAddress;
				*valuePtrAddress = 0;
			}
		}

		/** @param elementIndex  The element to get values for.
		  * @param valuesCount  The number of values expected. Used to distinguish scalar/vector storage.
		  * @return  Address of values, or 0 if none. Note can return address of a scalar when it doesn't
		  * exist. */
		const ValueType *getElementValues(DsLabelIndex elementIndex, int valuesCount) const
		{
			if (valuesCount == 1)
				return this->elementScalarDOFs.getAddress(elementIndex); // can be false positive
			return this->elementVectorDOFs.getValue(elementIndex);
		}

		/** @param elementIndex  The element to get values for.
		  * @param valuesCount  The number of values expected. Used to distinguish scalar/vector storage.
		  * @return  Address of values, or 0 if none. Note can return address of a scalar when it doesn't
		  * exist. */
		ValueType *getElementValues(DsLabelIndex elementIndex, int valuesCount)
		{
			if (valuesCount == 1)
				return this->elementScalarDOFs.getAddress(elementIndex); // can be false positive
			return this->elementVectorDOFs.getValue(elementIndex);
		}

		/** @param elementIndex  The element to get or create values for.
		  * @param valuesCount  The number of values required to store >= 1.
		  * @return  Pointer to values array or 0 if failed. Not to be deallocated! */
		ValueType *getOrCreateElementValues(DsLabelIndex elementIndex, int valuesCount)
		{
			if (valuesCount == 1)
				return this->elementScalarDOFs.getOrCreateAddress(elementIndex);
			if (valuesCount <= 0)
				return 0;
			ValueType **existingValuesAddress = this->elementVectorDOFs.getAddress(elementIndex);
			ValueType *values = (existingValuesAddress) ? *existingValuesAddress : 0;
			if (!values)
			{
				values = new ValueType[valuesCount];
				if (!values)
					return 0;
				if (existingValuesAddress)
				{
					*existingValuesAddress = values;
				}
				else if (!this->elementVectorDOFs.setValue(elementIndex, values))
				{
					delete values;
					return 0;
				}
			}
			return values;
		}

		/** @param elementIndex  The element to set values for.
		  * @param valuesCount  Size of values array; must be >= 1.
		  * @param values  Array of values to set. Must be non-null.
		  * @return  True on success, false on failure. */
		bool setElementValues(DsLabelIndex elementIndex, int valuesCount, const ValueType *values)
		{
			ValueType *targetValues = this->getOrCreateElementValues(elementIndex, valuesCount);
			if (!targetValues)
				return false;
			memcpy(targetValues, values, valuesCount*sizeof(ValueType));
			return true;
		}

		/** Copy element values from source component, finding target elements by identifier.
		  * Used only by FE_mesh::merge to merge values from another region.
		  * Must have already merged mesh field template for component, so target element
		  * field template is guaranteed to be the same for each element in source.
		  * @param sourceBase  Source component as base class. Must of same ValueType.
		  * @return  True on success, false on failure. */
		virtual bool mergeElementValues(const ComponentBase *sourceBase)
		{
			const Component *source = dynamic_cast<const Component<ValueType> *>(sourceBase);
			if (!source)
			{
				display_message(ERROR_MESSAGE, "FE_mesh_field_data::Component::mergeElementDOFs.  Invalid source");
				return false;
			}
			const FE_mesh *sourceMesh = source->meshFieldTemplate->getMesh();
			const FE_mesh *targetMesh = this->meshFieldTemplate->getMesh();
			DsLabelIndex sourceElementIndexLimit = source->meshFieldTemplate->getElementIndexLimit();
			for (DsLabelIndex sourceElementIndex = source->meshFieldTemplate->getElementIndexStart();
				sourceElementIndex < sourceElementIndexLimit; ++sourceElementIndex)
			{
				const DsLabelIdentifier elementIdentifier = sourceMesh->getElementIdentifier(sourceElementIndex);
				if (DS_LABEL_IDENTIFIER_INVALID == elementIdentifier)
					continue; // no element at that index
				FE_element_field_template *sourceElementfieldtemplate = source->meshFieldTemplate->getElementfieldtemplate(sourceElementIndex);
				if (!sourceElementfieldtemplate)
					continue;
				const int valuesCount = sourceElementfieldtemplate->getNumberOfElementDOFs();
				if (0 == valuesCount)
					continue;
				const ValueType *sourceValues = source->getElementValues(sourceElementIndex, valuesCount);
				if (!sourceValues)
				{
					display_message(ERROR_MESSAGE, "FE_mesh_field_data::Component::mergeElementDOFs.  Missing source DOFs");
					return false;
				}
				const DsLabelIndex targetElementIndex = targetMesh->findIndexByIdentifier(elementIdentifier);
				// the above must be valid if merge has got to this point, so not testing.
				if (!this->setElementValues(targetElementIndex, valuesCount, sourceValues))
					return false;
			}
			return true;
		}

	};

	/** Simple component type for types without element varying quantities e.g. indexed string */
	class ComponentConstant : public ComponentBase
	{
	public:

		ComponentConstant(FE_mesh_field_template *meshFieldTemplateIn) :
			ComponentBase(meshFieldTemplateIn)
		{
		}

		virtual ~ComponentConstant()
		{
		}

		virtual void clearElementData(DsLabelIndex)
		{
		}

		virtual bool mergeElementValues(const ComponentBase *)
		{
			return true;
		}

	};


private:
	FE_field *field; // not accessed; structure is owned by field and dies with it
	const int componentCount; // cached from field
	const Value_type valueType;
	ComponentBase **components;

	FE_mesh_field_data(FE_field *fieldIn, ComponentBase **componentsIn);

public:

	/** @return  New mesh field data with blank FieldTemplates for field on mesh, or 0 if failed */
	static FE_mesh_field_data *create(FE_field *field, FE_mesh *mesh);

	~FE_mesh_field_data()
	{
		for (int i = 0; i < this->componentCount; ++i)
			delete this->components[i];
		delete[] this->components;
	}

	/** @param componentNumber  From 0 to componentCount - 1, not checked.
	  * @param elementIndex  Element index >= 0. Not checked. */
	void clearComponentElementData(int componentNumber, DsLabelIndex elementIndex)
	{
		this->components[componentNumber]->clearElementData(elementIndex);
	}

	/** Clear element data for all components
	  * @param elementIndex  Element index >= 0. Not checked. */
	void clearElementData(DsLabelIndex elementIndex)
	{
		for (int c = 0; c < this->componentCount; ++c)
			this->components[c]->clearElementData(elementIndex);
	}

	/** @param componentNumber  From 0 to componentCount - 1, not checked
	  * @return  Non-accessed component base */
	ComponentBase *getComponentBase(int componentNumber) const
	{
		return this->components[componentNumber];
	}

	/** @param componentNumber  From 0 to componentCount - 1, not checked
	  * @return  Non-accessed field template */
	const FE_mesh_field_template *getComponentMeshfieldtemplate(int componentNumber) const
	{
		return this->components[componentNumber]->getMeshfieldtemplate();
	}

	/** @param componentNumber  From 0 to componentCount - 1, not checked
	  * @return  Non-accessed field template */
	FE_mesh_field_template *getComponentMeshfieldtemplate(int componentNumber)
	{
		return this->components[componentNumber]->getMeshfieldtemplate();
	}

	/** @param componentNumber  From 0 to componentCount - 1, not checked */
	void setComponentMeshfieldtemplate(int componentNumber, FE_mesh_field_template *meshFieldTemplate)
	{
		this->components[componentNumber]->setMeshfieldtemplate(meshFieldTemplate);
	}

	bool isDefinedOnElements()
	{
		return !this->components[0]->getMeshfieldtemplate()->isBlank();
	}

	/** @return  True if any element of any component uses a non-linear basis in any direction */
	bool usesNonLinearBasis() const
	{
		// since this is expensive, cache already checked linear mfts
		std::vector<const FE_mesh_field_template *> checkedMfts;
		for (int c = 0; c < this->componentCount; ++c)
		{
			const FE_mesh_field_template *mft = this->components[c]->getMeshfieldtemplate();
			if (std::find(checkedMfts.begin(), checkedMfts.end(), mft) != checkedMfts.end())
			{
				continue; // already checked
			}
			if (mft->usesNonLinearBasis())
			{
				return true;
			}
			checkedMfts.push_back(mft);
		}
		return false;
	}
};

#endif /* !defined (FINITE_ELEMENT_MESH_HPP) */
