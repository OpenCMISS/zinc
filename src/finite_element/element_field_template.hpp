/**
 * FILE : element_field_template.hpp
 *
 * Describes parameter mapping and interpolation for a scalar
 * field or field component over an element.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CMZN_ELEMENT_FIELD_TEMPLATE_HPP)
#define CMZN_ELEMENT_FIELD_TEMPLATE_HPP

#include "opencmiss/zinc/types/elementid.h"
#include "opencmiss/zinc/types/elementfieldtemplateid.h"
#include "opencmiss/zinc/types/nodeid.h"
#include "opencmiss/zinc/status.h"
#include "general/value.h"
#include "finite_element/finite_element_basis.hpp"
#include "datastore/labels.hpp"
#include <vector>

struct FE_basis;
struct FE_field;
class FE_mesh;
class FE_nodeset;
class FE_node_field_template;


inline bool isScaleFactorTypeElement(cmzn_elementfieldtemplate_scale_factor_type scaleFactorType)
{
	return (scaleFactorType == CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_GENERAL)
		|| (scaleFactorType == CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_PATCH);
}

inline bool isScaleFactorTypeGlobal(cmzn_elementfieldtemplate_scale_factor_type scaleFactorType)
{
	return (scaleFactorType == CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_GENERAL)
		|| (scaleFactorType == CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_PATCH);
}

inline bool isScaleFactorTypeNode(cmzn_elementfieldtemplate_scale_factor_type scaleFactorType)
{
	return (scaleFactorType == CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_GENERAL)
		|| (scaleFactorType == CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_PATCH);
}


class FE_element_field_template
{
	friend class FE_mesh;

	friend int global_to_element_map_values(FE_field *field, int componentNumber,
		const FE_element_field_template *eft, cmzn_element *element, FE_value time,
		const FE_nodeset *nodeset, const FE_value *scaleFactors, FE_value*& elementValues);

	friend int global_to_element_map_nodes(FE_field *field, int componentNumber,
		const FE_element_field_template *eft, cmzn_element *element,
		int &basisNodeCount, DsLabelIndex *&basisNodeIndexes);

private:

	FE_mesh *mesh; // not accessed; mesh maintains EFT list to clear this when destroyed. Must check non-zero before use
	const int dimension; // cached from mesh, basis
	FE_basis *basis; // accessed
	const int numberOfFunctions; // number of basis functions, cached from basis

	bool locked; // set once in use by mesh/field or element template, so not modifiable
	int indexInMesh; // if in use by mesh, the index (>=0) of its FE_mesh_element_field_template_data in mesh. Otherwise -1

	cmzn_elementfieldtemplate_parameter_mapping_mode mappingMode; // ELEMENT|FIELD|NODE

	// for each basis function: number of terms summed to give parameter;
	// currently only supported for node mapping mode; all others have 1 term:
	int *termCounts;
	int totalTermCount; // sum of termCounts[fn] for all functions
	// for each basis function, offset into term arrays for the first term with others following
	// equals cummulative value of termCounts up to that function number:
	int *termOffsets;

	// node mapping information:
	int numberOfLocalNodes; // non-zero only for node mapping mode
	// packed arrays of termCounts[fn] local node index, value label and version
	// for each function, in order of function number (size = totalTermCount, looked up by termOffsets):
	int *localNodeIndexes;
	cmzn_node_value_label *nodeValueLabels;
	int *nodeVersions;

	// scale factor information: each term can be multiplied by zero or more scale factors
	int numberOfLocalScaleFactors; // zero if unscaled; scaling currently only supported with node mapping
	// metadata identifying type and version of each local scale factor, for merging:
	cmzn_elementfieldtemplate_scale_factor_type *scaleFactorTypes;
	int *scaleFactorIdentifiers;
	// determined on validation: if any node scale factors in use, allocate and cache local node indexes for each.
	// It is an error if a node scale factor is not for exactly one node
	int *scaleFactorLocalNodeIndexes;

	// packed array of all local scale factor indexes to be multiplied for each
	// term for each function, in order of function then term then factor (size = totalLocalScaleFactorIndexes):
	int *localScaleFactorIndexes;
	// packed arrays of termCounts[fn] number of scale factors to multiply, and
	// offsets into above array where the indexes are held (size = totalTermCount, looked up by termOffsets):
	int *termScaleFactorCounts;
	int *termLocalScaleFactorIndexesOffsets;
	// sum of termScaleFactorCounts[termOffsets[fn] + term] = size of localScaleFactorIndexes array:
	int totalLocalScaleFactorIndexes;

	// for element parameter mapping only, number of linear cells in each xi direction, or 0 for constant
	int *legacyGridNumberInXi;

	// optional function for modifying element values before use, e.g. to handle wrapping around polar coordinates
	// @see FE_basis_modify_theta_in_xi1
	// this is a dirty hack and not for public API, for translating legacy EX files only
	FE_basis_modify_theta_mode legacyModifyThetaMode;

	// data computed only on validation, for working with field parameters:
	// note that element component parameters are in the order first used by function terms
	// note that arrays are not allocated for default case where all functions have one term
	int parameterCount;
	int *parameterTermCounts;  // number of function terms multiplying each parameter
	int *parameterTermOffsets;  // for each parameter, offset into parameterFunctionTerms where it's values are held, multiples of 2
	int parameterFunctionTermsSize;  // size of parameterFunctionTerms, multiple of 2 for the pairs
	int *parameterFunctionTerms;  // packed array of pairs (function index, term index) for each parameter term

	int access_count;

	FE_element_field_template(FE_mesh *meshIn, FE_basis *basisIn);

	FE_element_field_template(const FE_element_field_template &source);

	~FE_element_field_template();

	FE_element_field_template &operator=(const FE_element_field_template &source); // not implemented

	void clearNodeMapping();

	void clearParameterMaps();

	void clearScaling();

	inline bool validTerm(int functionNumber, int term) const
	{
		return (0 <= functionNumber) && (functionNumber < this->numberOfFunctions) &&
			(0 <= term) && (term < this->termCounts[functionNumber]);
	}

	/** Only to be called by FE_mesh */
	void setIndexInMesh(FE_mesh *meshIn, int indexInMeshIn);

	bool convertNodeParameterLegacyIndexes(std::vector<int> &legacyDOFIndexes,
		std::vector<const FE_node_field_template*> &nodeFieldTemplates);

	bool checkNodeParameterLegacyIndexes(std::vector<int> &legacyDOFIndexes,
		std::vector<const FE_node_field_template*> &nodeFieldTemplates);

	/** Called by ~FE_mesh */
	void detachFromMesh()
	{
		this->mesh = 0;
	}

public:

	static FE_element_field_template *create(FE_mesh *meshIn, FE_basis *basisIn);

	/** Create an unlocked copy of the template for editing.
	  * @return  New element field template with access count 1, or 0 if failed. */
	FE_element_field_template *cloneForModify() const;

	/** Create a copy of the template for merging into a new mesh, keeping locked status.
	  * @param newMesh  New mesh to clone for. Must be same dimension as this eft's mesh.
	  * @return  New element field template with access count 1, or 0 if failed. */
	FE_element_field_template *cloneForNewMesh(FE_mesh *newMesh) const;

	FE_element_field_template *access()
	{
		++access_count;
		return this;
	}

	static int deaccess(FE_element_field_template* &eft)
	{
		if (!eft)
			return CMZN_ERROR_ARGUMENT;
		--(eft->access_count);
		if (eft->access_count <= 0)
			delete eft;
		eft = 0;
		return CMZN_OK;
	}

	int getIndexInMesh() const
	{
		return this->indexInMesh;
	}

	/** @return  True if Returns true if this template and source are identically defined */
	bool matches(const FE_element_field_template &source) const;

	/** Returns true if locked i.e. in use and read only; caller should clone to modify */
	bool isLocked() const
	{
		return this->locked;
	}

	/** @return  Boolean true if this element field template has been merged into its mesh. */
	bool isMergedInMesh() const
	{
		return this->indexInMesh >= 0;
	}

	FE_basis *getBasis() const
	{
		return this->basis;
	}

	cmzn_elementfieldtemplate_parameter_mapping_mode getParameterMappingMode() const
	{
		return this->mappingMode;
	}

	/** Sets the parameter mapping mode. Can only set CONSTANT mapping with constant basis,
	  * mapping a single constant value for a given field component in every element using EFT.
	  * For other mapping modes, resets to one term per basis function with no scaling hence
	  * should be the first setting changed. */
	int setParameterMappingMode(cmzn_elementfieldtemplate_parameter_mapping_mode modeIn);

	/** @return  Highest local node index used in EFT, or -1 if not node-based. */
	int getHighestLocalNodeIndex() const;

	/** @return  Non-accessed pointer to the mesh for this template, or NULL if mesh destroyed. */
	FE_mesh *getMesh() const
	{
		return this->mesh;
	}

	/** @return  For element-based parameters, array containing number of grid
	  * cells in each xi direction  >= 0, or 0 if not a grid. For immediate use only */
	const int *getLegacyGridNumberInXi() const
	{
		return this->legacyGridNumberInXi;
	}

	int setLegacyGridNumberInXi(const int *numberInXiIn);

	FE_basis_modify_theta_mode getLegacyModifyThetaMode() const
	{
		return this->legacyModifyThetaMode;
	}

	/** Only valid for PARAMETER_MAPPING_MODE_NODE */
	int setLegacyModifyThetaMode(FE_basis_modify_theta_mode modifyThetaModeIn);

	int getNumberOfElementDOFs() const;

	bool hasElementDOFs() const
	{
		return (this->mappingMode == CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_ELEMENT);
	}

	/** @return  Number of basis functions */
	int getNumberOfFunctions() const
	{
		return this->numberOfFunctions;
	}

	int getNumberOfLocalNodes() const
	{
		return this->numberOfLocalNodes;
	}

	/** Set the number of local nodes. If reducing number, template is only valid
	  * once all indexes are in range 0..number-1.
	  * Only valid in node mapping mode. */
	int setNumberOfLocalNodes(int number);

	/** Get the number of terms that are summed to give the element parameter weighting
	  * the given function number.
	  * @param functionNumber  Basis function number from 0 to numberOfFunctions - 1.
	  * @return  Number of terms >= 0, or -1 if invalid function number */
	int getFunctionNumberOfTerms(int functionNumber) const
	{
		if ((functionNumber < 0) || (functionNumber >= this->numberOfFunctions))
			return -1; // since can be zero terms
		return this->termCounts[functionNumber];
	}

	/** Set the number of terms that are summed to give the element parameter weighting
	  * the given function number. Currently only supported for node mapping.
	  * If reducing number, existing mappings for higher terms are discarded.
	  * If increasing number, new mappings must be completely specified by subsequent
	  * calls; new mappings are unscaled by default.
	  * @param functionNumber  Basis function number from 0 to numberOfFunctions - 1.
	  * @param newNumberOfTerms  New number of terms to be summed, >= 0.
	  * @return  Result OK on success, otherwise error code */
	int setFunctionNumberOfTerms(int functionNumber, int newNumberOfTerms);

	/** @return  Local node index starting at 0, or -1 if invalid function or term */
	int getTermLocalNodeIndex(int functionNumber, int term) const;

	/** @return  Node value label or INVALID if not set or invalid function or term */
	cmzn_node_value_label getTermNodeValueLabel(int functionNumber, int term) const;

	/** @return  Version starting at 0, or -1 if invalid function or term */
	int getTermNodeVersion(int functionNumber, int term) const;

	/** @return  Legacy DOF index starting at 0, or -1 if invalid function or term or not a legacy index */
	int getTermNodeLegacyIndex(int functionNumber, int term) const;

	/** For node mapping mode, set the given term for function number to look up
	  * the value of the node value label & version at local node index.
	  * @param functionNumber Basis function number from 0 to numberOfFunctions - 1.
	  * @param term  Term number from 0 to function number of terms - 1 */
	int setTermNodeParameter(int functionNumber, int term, int localNodeIndex, cmzn_node_value_label valueLabel, int version);

	/** Internal-only function for setting legacy parameter mapping by DOF index.
	  * Used only for reading legacy EX format. Converted to value label + version prior to merge.
	  * @param functionNumber Basis function number from 0 to numberOfFunctions - 1.
	  * @param term  Term number from 0 to function number of terms - 1.
	  * @param nodeDOFIndex  Index of DOF for field component at node, starting at 0. */
	int setTermNodeParameterLegacyIndex(int functionNumber, int term, int localNodeIndex, int nodeDOFIndex);

	/** @return  True if template maps any node parameters with legacy node parameter indexes
	  * @see setTermNodeParameterLegacyIndex */
	bool hasNodeParameterLegacyIndex() const;

	int getNumberOfLocalScaleFactors() const
	{
		return this->numberOfLocalScaleFactors;
	}

	/** Set the number of local scale factors. If reducing number, template is
	  * only valid once all indexes are in range 0..number-1.
	  * Only valid in node mapping mode. */
	int setNumberOfLocalScaleFactors(int number);

	cmzn_elementfieldtemplate_scale_factor_type getScaleFactorType(int localScaleFactorIndex) const
	{
		if ((0 <= localScaleFactorIndex) && (localScaleFactorIndex < this->numberOfLocalScaleFactors))
			return this->scaleFactorTypes[localScaleFactorIndex];
		return CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_INVALID;
	}

	/** Set the type of scale factor mapped at the local index, used with the
	  * scale factor version, and node/element to merge common scale factors
	  * across the mesh */
	int setScaleFactorType(int localScaleFactorIndex, cmzn_elementfieldtemplate_scale_factor_type type);

	int getScaleFactorIdentifier(int localScaleFactorIndex) const
	{
		if ((0 <= localScaleFactorIndex) && (localScaleFactorIndex < this->numberOfLocalScaleFactors))
			return this->scaleFactorIdentifiers[localScaleFactorIndex];
		return -1;
	}

	/** Set the identifier of the scale factor mapped at the local index, used
	  * with the scale factor type, and node/element to merge common scale factors
	  * across the mesh */
	int setScaleFactorIdentifier(int localScaleFactorIndex, int identifier);

	/** Get the local node index from which the scale factor is mapped, if any.
	  * @return  Local node index >= 0, or -1 if none. */
	int getScaleFactorLocalNodeIndex(int localScaleFactorIndex)
	{
		if ((this->scaleFactorLocalNodeIndexes)
			&& (0 <= localScaleFactorIndex)
			&& (localScaleFactorIndex < this->numberOfLocalScaleFactors))
		{
			return this->scaleFactorLocalNodeIndexes[localScaleFactorIndex];
		}
		return -1;
	}

	/** @return  The number of scale factors multiplying term */
	int getTermScalingCount(int functionNumber, int term) const;

	/** @param termScaleFactorIndex  From 0 to term scaling count.
	  * @return  Index of scale factor, or -1 on error. */
	int getTermScaleFactorIndex(int functionNumber, int term, int termScaleFactorIndex) const;

	/** @param startIndex  Set to 1 to offset to external indexes which start at 1
	  * @return  The actual number of scaling indexes for term, or -1 on error. */
	int getTermScaling(int functionNumber, int term, int indexesCount, int *indexesOut, int startIndex = 0) const;

	/** Set scaling of the function term by the product of scale factors at the
	  * given local scale factor indexes. Only valid in node mapping mode.
	  * Must have set positive number of local scale factors.
	  * @param startIndex  Set to 1 to offset from external indexes which start at 1 */
	int setTermScaling(int functionNumber, int term, int indexesCount, const int *indexesIn, int startIndex = 0);

	/** Remap local node indexes to map extNodeIndexes when sorted from lowest to highest.
	  * @param extNodeIndexes  Array of unique, non-negative indexes for current EFT local indexes.
	  * Must be large enough for number of local nodes in use. On successful return this array is
	  * sorted from lowest to highest to match EFT internal reference order.
	  * e.g. if extNodeIndexes are 2 6 3 1, they are sorted to 1 2 3 6, and internal indexes
	  * 0 1 2 3 are remapped to 1 3 2 0, respectively.
	  * @return  Result OK on success, any other value on failure. */
	int sortNodeIndexes(std::vector<int>& extNodeIndexes);

	int getTotalTermCount() const
	{
		return this->totalTermCount;
	}

	/** Get number of parameters in use by element field template.
	 * Only available when validated. */
	int getParameterCount() const
	{
		return this->parameterCount;
	}

	/** Get number of terms multiplying the parameter.
	 * Only available when validated.
	 * @param parameterIndex  Index of parameter to query, starting at 0
	 * @return  Number of terms > 0, or 0 if failed. */
	int getParameterTermCount(int parameterIndex) const;

	/** Get function and term for one parameter term multiplying the parameter.
	 * Only available when validated.
	 * @param parameterIndex  Index of parameter to query, starting at 0
	 * @param parameterTerm  Parameter term, from 0 to 1 less than
	 * getParameterTermCount(parameterIndex).
	 * @param term  On success returns the function term for the parameter term.
	 * @return  Function index for parameter term, or -1 if failed */
	int getParameterTermFunctionAndTerm(int parameterIndex, int parameterTerm, int &term) const;

	/** @return  True if validated and locked, false if failed. */
	bool validateAndLock();

	/** @return  True if any node-based scale factors in use, requiring client to update if local nodes change */
	bool hasNodeScaleFactors() const
	{
		return (0 != this->scaleFactorLocalNodeIndexes);
	}

};

struct cmzn_elementfieldtemplate
{
private:

	FE_element_field_template *impl;
	int access_count;

	cmzn_elementfieldtemplate(FE_element_field_template *implIn) :
		impl(implIn), // take ownership of access
		access_count(1)
	{
	}

	~cmzn_elementfieldtemplate()
	{
		FE_element_field_template::deaccess(this->impl);
	}

	/** Call before modifying object to ensure it is either unlocked or an unlocked copy is created */
	void copyOnWrite()
	{
		if (this->impl->isLocked())
		{
			FE_element_field_template *tmp = this->impl;
			this->impl = tmp->cloneForModify();
			FE_element_field_template::deaccess(tmp);
		}
	}

	/** Tries to substitute implementation with equivalent merged EFT.
	  * @return  True if merged EFT is substituted, otherwise false if no match found.*/
	bool findMergedInMesh();

public:

	static cmzn_elementfieldtemplate *create(FE_mesh *meshIn, FE_basis *basisIn)
	{
		FE_element_field_template *impl = FE_element_field_template::create(meshIn, basisIn);
		if (impl)
			return new cmzn_elementfieldtemplate(impl);
		return 0;
	}

	static cmzn_elementfieldtemplate *create(FE_element_field_template *implIn)
	{
		if (implIn)
			return new cmzn_elementfieldtemplate(implIn->access());
		return 0;
	}

	cmzn_elementfieldtemplate *access()
	{
		++access_count;
		return this;
	}

	static int deaccess(cmzn_elementfieldtemplate* &eft)
	{
		if (!eft)
			return CMZN_ERROR_ARGUMENT;
		--(eft->access_count);
		if (eft->access_count <= 0)
			delete eft;
		eft = 0;
		return CMZN_OK;
	}

	/** @return  True if Returns true if this template and source are identically defined */
	bool matches(const cmzn_elementfieldtemplate &source) const
	{
		return this->impl->matches(*(source.impl));
	}

	FE_basis *getBasis() const
	{
		return this->impl->getBasis();
	}

	cmzn_elementbasis *getElementbasis() const;

	cmzn_elementfieldtemplate_parameter_mapping_mode getParameterMappingMode() const
	{
		return this->impl->getParameterMappingMode();
	}

	int setParameterMappingMode(cmzn_elementfieldtemplate_parameter_mapping_mode mode)
	{
		this->copyOnWrite();
		return this->impl->setParameterMappingMode(mode);
	}

	/** get handle to implementation, first trying to substitute merged equivalent */
	FE_element_field_template *get_FE_element_field_template()
	{
		if (!this->impl->isMergedInMesh())
		{
			this->findMergedInMesh();
		}
		return this->impl;
	}

	/** @return  Non-accessed pointer to the mesh for this template. */
	FE_mesh *getMesh() const
	{
		return this->impl->getMesh();
	}

	const int *getLegacyGridNumberInXi() const
	{
		return this->impl->getLegacyGridNumberInXi();
	}

	int setLegacyGridNumberInXi(const int *numberInXiIn)
	{
		this->copyOnWrite();
		return this->impl->setLegacyGridNumberInXi(numberInXiIn);
	}

	FE_basis_modify_theta_mode getLegacyModifyThetaMode() const
	{
		return this->impl->getLegacyModifyThetaMode();
	}

	int setLegacyModifyThetaMode(FE_basis_modify_theta_mode modifyThetaModeIn)
	{
		this->copyOnWrite();
		return this->impl->setLegacyModifyThetaMode(modifyThetaModeIn);
	}

	int getNumberOfFunctions() const
	{
		return this->impl->getNumberOfFunctions();
	}

	int getNumberOfLocalNodes() const
	{
		return this->impl->getNumberOfLocalNodes();
	}

	int setNumberOfLocalNodes(int number)
	{
		this->copyOnWrite();
		return this->impl->setNumberOfLocalNodes(number);
	}

	int getFunctionNumberOfTerms(int functionNumber) const
	{
		return this->impl->getFunctionNumberOfTerms(functionNumber - 1);
	}

	int setFunctionNumberOfTerms(int functionNumber, int newNumberOfTerms)
	{
		this->copyOnWrite();
		return this->impl->setFunctionNumberOfTerms(functionNumber - 1, newNumberOfTerms);
	}

	int getTermLocalNodeIndex(int functionNumber, int term) const
	{
		return this->impl->getTermLocalNodeIndex(functionNumber - 1, term - 1) + 1;
	}

	cmzn_node_value_label getTermNodeValueLabel(int functionNumber, int term) const
	{
		return this->impl->getTermNodeValueLabel(functionNumber - 1, term - 1);
	}

	int getTermNodeVersion(int functionNumber, int term) const
	{
		return this->impl->getTermNodeVersion(functionNumber - 1, term - 1) + 1;
	}

	int setTermNodeParameter(int functionNumber, int term, int localNodeIndex, cmzn_node_value_label valueLabel, int version)
	{
		this->copyOnWrite();
		return this->impl->setTermNodeParameter(functionNumber - 1, term - 1, localNodeIndex - 1, valueLabel, version - 1);
	}

	int setTermNodeParameterLegacyIndex(int functionNumber, int term, int localNodeIndex, int nodeDOFIndex)
	{
		this->copyOnWrite();
		return this->impl->setTermNodeParameterLegacyIndex(functionNumber - 1, term - 1, localNodeIndex - 1, nodeDOFIndex - 1);
	}

	int getNumberOfLocalScaleFactors() const
	{
		return this->impl->getNumberOfLocalScaleFactors();
	}

	int setNumberOfLocalScaleFactors(int number)
	{
		this->copyOnWrite();
		return this->impl->setNumberOfLocalScaleFactors(number);
	}

	cmzn_elementfieldtemplate_scale_factor_type getScaleFactorType(int localScaleFactorIndex) const
	{
		return this->impl->getScaleFactorType(localScaleFactorIndex - 1);
	}

	int setScaleFactorType(int localScaleFactorIndex, cmzn_elementfieldtemplate_scale_factor_type type)
	{
		this->copyOnWrite();
		return this->impl->setScaleFactorType(localScaleFactorIndex - 1, type);
	}

	int getScaleFactorIdentifier(int localScaleFactorIndex) const
	{
		return this->impl->getScaleFactorIdentifier(localScaleFactorIndex - 1);
	}

	int setScaleFactorIdentifier(int localScaleFactorIndex, int identifier)
	{
		this->copyOnWrite();
		return this->impl->setScaleFactorIdentifier(localScaleFactorIndex - 1, identifier);
	}

	int getTermScaling(int functionNumber, int term, int indexesCount, int *indexesOut) const
	{
		return this->impl->getTermScaling(functionNumber - 1, term - 1, indexesCount, indexesOut, /*startIndex*/1);
	}

	int setTermScaling(int functionNumber, int term, int indexesCount, const int *indexesIn)
	{
		this->copyOnWrite();
		return this->impl->setTermScaling(functionNumber - 1, term - 1, indexesCount, indexesIn, /*startIndex*/1);
	}

	int sortNodeIndexes(std::vector<int>& extNodeIndexes)
	{
		this->copyOnWrite();
		return this->impl->sortNodeIndexes(extNodeIndexes);
	}

	/** @return  True if validated and locked, otherwise false. */
	bool validateAndLock()
	{
		return this->impl->validateAndLock();
	}

};

#endif /* !defined (CMZN_ELEMENT_FIELD_TEMPLATE_HPP) */
