/**
 * FILE : element_field_template.cpp
 *
 * Describes parameter mapping and interpolation for a scalar
 * field or field component over an element.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/elementfieldtemplate.h"
#include "opencmiss/zinc/mesh.h"
#include "finite_element/element_field_template.hpp"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_private.h"
#include "finite_element/node_field_template.hpp"
#include "general/message.h"
#include "mesh/cmiss_element_private.hpp"
#include "mesh/cmiss_node_private.hpp"

#include <algorithm>
#include <cstring>

namespace {

	/** @return  Allocated copy of sourceValues array containing count values, or 0 if none */
	template<typename VALUETYPE> VALUETYPE *copy_array(VALUETYPE *sourceValues, int count)
	{
		if (!sourceValues)
			return nullptr;
		VALUETYPE *values = new VALUETYPE[count];
		memcpy(values, sourceValues, count*sizeof(VALUETYPE));
		return values;
	}

	/** Ensure values array is large enough for newSize,
	  * with copy of values up to lowest of newIndex or oldIndex,
	  * copy (newSize-newIndex) values from oldIndex to newIndex,
	  * if growing (newIndex > oldIndex) set any new values to fillValue. */
	template<typename VALUETYPE> void edit_array(
		VALUETYPE *&values, int newIndex, int oldIndex, int newSize, VALUETYPE fillValue)
	{
		if (values)
		{
			const size_t trailingBytesToCopy = (newSize - newIndex)*sizeof(VALUETYPE);
			if (newIndex > oldIndex)
			{
				VALUETYPE *oldValues = values;
				values = new VALUETYPE[newSize];
				if (oldIndex)
					memcpy(values, oldValues, /*leadingBytesToCopy*/oldIndex*sizeof(VALUETYPE));
				for (int i = oldIndex; i < newIndex; ++i)
					values[i] = fillValue;
				if (trailingBytesToCopy)
					memcpy(values + newIndex, oldValues + oldIndex, trailingBytesToCopy);
				delete[] oldValues;
			}
			else
			{
				if (trailingBytesToCopy)
					memmove(values + newIndex, values + oldIndex, trailingBytesToCopy);
			}
		}
	}

	inline bool valid_node_value_label(cmzn_node_value_label valueLabel)
	{
		return (CMZN_NODE_VALUE_LABEL_VALUE <= valueLabel) && (valueLabel <= CMZN_NODE_VALUE_LABEL_D3_DS1DS2DS3);
	}

} // anonymous namespace

FE_element_field_template::FE_element_field_template(FE_mesh *meshIn, FE_basis *basisIn) :
	mesh(meshIn),
	dimension(meshIn->getDimension()),
	basis(ACCESS(FE_basis)(basisIn)),
	numberOfFunctions(FE_basis_get_number_of_functions(basisIn)),
	locked(false),
	indexInMesh(-1),
	mappingMode(CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_INVALID),
	termCounts(new int[this->numberOfFunctions]),
	totalTermCount(0),
	termOffsets(new int[this->numberOfFunctions]),
	numberOfLocalNodes(0),
	localNodeIndexes(0),
	nodeValueLabels(0),
	nodeVersions(0),
	numberOfLocalScaleFactors(0),
	scaleFactorTypes(0),
	scaleFactorIdentifiers(0),
	scaleFactorLocalNodeIndexes(0),
	localScaleFactorIndexes(0),
	termScaleFactorCounts(0),
	termLocalScaleFactorIndexesOffsets(0),
	totalLocalScaleFactorIndexes(0),
	legacyGridNumberInXi(0),
	legacyModifyThetaMode(FE_BASIS_MODIFY_THETA_MODE_INVALID),
	parameterCount(0),
	parameterTermCounts(nullptr),
	parameterTermOffsets(nullptr),
	parameterFunctionTermsSize(0),
	parameterFunctionTerms(nullptr),
	access_count(1)
{
	this->setParameterMappingMode(CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE);
	// FE_mesh maintains a list of these to clear mesh pointer when mesh is destroyed
	this->mesh->addElementfieldtemplate(this);
}

FE_element_field_template::FE_element_field_template(const FE_element_field_template &source) :
	mesh(source.mesh),
	dimension(source.dimension),
	basis(ACCESS(FE_basis)(source.basis)),
	numberOfFunctions(source.numberOfFunctions),
	locked(source.locked),
	indexInMesh(-1),
	mappingMode(source.mappingMode),
	termCounts(copy_array(source.termCounts, source.numberOfFunctions)),
	totalTermCount(source.totalTermCount),
	termOffsets(copy_array(source.termOffsets, source.numberOfFunctions)),
	numberOfLocalNodes(source.numberOfLocalNodes),
	localNodeIndexes(copy_array(source.localNodeIndexes, source.totalTermCount)),
	nodeValueLabels(copy_array(source.nodeValueLabels, source.totalTermCount)),
	nodeVersions(copy_array(source.nodeVersions, source.totalTermCount)),
	numberOfLocalScaleFactors(source.numberOfLocalScaleFactors),
	scaleFactorTypes(copy_array(source.scaleFactorTypes, source.numberOfLocalScaleFactors)),
	scaleFactorIdentifiers(copy_array(source.scaleFactorIdentifiers, source.numberOfLocalScaleFactors)),
	scaleFactorLocalNodeIndexes(source.scaleFactorLocalNodeIndexes ?
		copy_array(source.scaleFactorLocalNodeIndexes, source.numberOfLocalScaleFactors) : 0),
	localScaleFactorIndexes(copy_array(source.localScaleFactorIndexes, source.totalLocalScaleFactorIndexes)),
	termScaleFactorCounts(copy_array(source.termScaleFactorCounts, source.totalTermCount)),
	termLocalScaleFactorIndexesOffsets(copy_array(source.termLocalScaleFactorIndexesOffsets, source.totalTermCount)),
	totalLocalScaleFactorIndexes(source.totalLocalScaleFactorIndexes),
	legacyGridNumberInXi(copy_array(source.legacyGridNumberInXi, source.dimension)),
	legacyModifyThetaMode(source.legacyModifyThetaMode),
	parameterCount(source.parameterCount),
	parameterTermCounts(copy_array(source.parameterTermCounts, source.parameterCount)),
	parameterTermOffsets(copy_array(source.parameterTermOffsets, source.parameterCount)),
	parameterFunctionTermsSize(source.parameterFunctionTermsSize),
	parameterFunctionTerms(copy_array(source.parameterFunctionTerms, source.parameterFunctionTermsSize)),
	access_count(1)
{
	if (this->mesh) // may already be orphaned
		this->mesh->addElementfieldtemplate(this);
}

FE_element_field_template::~FE_element_field_template()
{
	if (this->mesh)
		this->mesh->removeElementfieldtemplate(this);
	DEACCESS(FE_basis)(&(this->basis));
	delete[] this->termCounts;
	delete[] this->termOffsets;
	this->clearNodeMapping();
	this->clearParameterMaps();
	this->clearScaling();
	delete this->legacyGridNumberInXi;
}

void FE_element_field_template::clearNodeMapping()
{
	this->numberOfLocalNodes = 0;
	delete[] this->localNodeIndexes;
	this->localNodeIndexes = 0;
	delete[] this->nodeValueLabels;
	this->nodeValueLabels = 0;
	delete[] this->nodeVersions;
	this->nodeVersions = 0;
}

void FE_element_field_template::clearParameterMaps()
{
	this->parameterCount = 0;
	delete[] this->parameterTermCounts;
	this->parameterTermCounts = nullptr;
	delete[] this->parameterTermOffsets;
	this->parameterTermOffsets = nullptr;
	this->parameterFunctionTermsSize = 0;
	delete[] this->parameterFunctionTerms;
	this->parameterFunctionTerms = nullptr;
}

void FE_element_field_template::clearScaling()
{
	this->numberOfLocalScaleFactors = 0;
	delete[] this->scaleFactorTypes;
	this->scaleFactorTypes = 0;
	delete[] this->scaleFactorIdentifiers;
	this->scaleFactorIdentifiers = 0;
	delete[] this->scaleFactorLocalNodeIndexes;
	this->scaleFactorLocalNodeIndexes = 0;
	delete[] this->localScaleFactorIndexes;
	this->localScaleFactorIndexes = 0;
	delete[] this->termScaleFactorCounts;
	this->termScaleFactorCounts = 0;
	delete[] this->termLocalScaleFactorIndexesOffsets;
	this->termLocalScaleFactorIndexesOffsets = 0;
}

void FE_element_field_template::setIndexInMesh(FE_mesh *meshIn, int indexInMeshIn)
{
	if (meshIn != this->mesh)
		display_message(ERROR_MESSAGE, "FE_element_field_template::setIndexInMesh  Invalid mesh");
	if (indexInMeshIn < -1)
		display_message(ERROR_MESSAGE, "FE_element_field_template::setIndexInMesh  Invalid index");
	else if (!this->locked)
		display_message(ERROR_MESSAGE, "FE_element_field_template::setIndexInMesh  Template is not locked");
	else if ((indexInMeshIn >= 0) && (this->indexInMesh >= 0))
		display_message(ERROR_MESSAGE, "FE_element_field_template::setIndexInMesh  Template already has an index in mesh");
	this->indexInMesh = indexInMeshIn;
}

/** For private use by FE_mesh::checkConvertLegacyNodeParameters.
  * Convert legacy node DOF indexes into value labels and versions.
  * @param legacyDOFIndexes  Legacy node DOF indexes previously copied out of versions array
  * @param nodeFieldComponents  Definitions of node fields at the local nodes, where
  * node value labels and versions are described for DOF indexes. */
bool FE_element_field_template::convertNodeParameterLegacyIndexes(
	std::vector<int> &legacyDOFIndexes, std::vector<const FE_node_field_template*> &nodeFieldTemplates)
{
	for (int tt = 0; tt < this->totalTermCount; ++tt)
	{
		const FE_node_field_template *nodeFieldComponent = nodeFieldTemplates[this->localNodeIndexes[tt]];
		const int dofIndex = legacyDOFIndexes[tt];
		if (0 <= dofIndex) // negative values mean new-style maps: no check
		{
			if (!nodeFieldComponent->convertLegacyDOFIndexToValueLabelAndVersion(dofIndex, this->nodeValueLabels[tt], this->nodeVersions[tt]))
			{
				display_message(ERROR_MESSAGE, "Element field template:  Failed to convert legacy node "
					"DOF index %d for term %d into value label and version.", dofIndex + 1, tt + 1);
				return false;
			}
		}
	}
	return true;
}

/** For private use by FE_mesh::checkConvertLegacyNodeParameters.
  * Check legacy node DOF indexes converted into value labels and versions matches eft content.
  * @param legacyDOFIndexes  Legacy node DOF indexes previously copied out of versions array
  * @param nodeFieldComponents  Definitions of node fields at the local nodes, where
  * node value labels and versions are described for DOF indexes. */
bool FE_element_field_template::checkNodeParameterLegacyIndexes(
	std::vector<int> &legacyDOFIndexes, std::vector<const FE_node_field_template*> &nodeFieldTemplates)
{
	for (int tt = 0; tt < this->totalTermCount; ++tt)
	{
		const FE_node_field_template *nodeFieldComponent = nodeFieldTemplates[this->localNodeIndexes[tt]];
		const int dofIndex = legacyDOFIndexes[tt];
		if (0 <= dofIndex) // negative values mean new-style maps: no check
		{
			cmzn_node_value_label valueLabel;
			int version;
			if (!nodeFieldComponent->convertLegacyDOFIndexToValueLabelAndVersion(dofIndex, valueLabel, version))
			{
				display_message(ERROR_MESSAGE, "Element field template:  Failed to convert legacy node "
					"DOF index %d for term %d into value label and version.", dofIndex + 1, tt + 1);
				return false;
			}
			if ((this->nodeValueLabels[tt] != valueLabel) || (this->nodeVersions[tt] != version))
			{
				display_message(ERROR_MESSAGE, "Element field template:  Value label and/or version for legacy node "
					"DOF index %d in element term %d does not match prior labelling for element field template.",
					dofIndex + 1, tt + 1);
				return false;
			}
		}
	}
	return true;
}

FE_element_field_template *FE_element_field_template::create(FE_mesh *meshIn, FE_basis *basisIn)
{
	int basisDimension = 0;
	FE_basis_get_dimension(basisIn, &basisDimension);
	if (meshIn && (basisDimension == meshIn->getDimension()))
		return new FE_element_field_template(meshIn, basisIn);
	display_message(ERROR_MESSAGE, "FE_element_field_template::create  Invalid argument(s)");
	return 0;
}

FE_element_field_template *FE_element_field_template::cloneForModify() const
{
	FE_element_field_template *eft = new FE_element_field_template(*this);
	if (!eft)
	{
		display_message(ERROR_MESSAGE, "FE_element_field_template::cloneForModify.  Failed");
		return 0;
	}
	eft->locked = false;
	// clear any copied data which should be computed on validation:
	delete eft->scaleFactorLocalNodeIndexes;
	eft->scaleFactorLocalNodeIndexes = 0;
	return eft;
}

FE_element_field_template *FE_element_field_template::cloneForNewMesh(FE_mesh *newMesh) const
{
	if (!this->mesh)
	{
		display_message(ERROR_MESSAGE, "FE_element_field_template::cloneForNewMesh.  No current mesh");
		return 0;
	}
	if ((!newMesh) || (newMesh->getDimension() != this->mesh->getDimension()))
	{
		display_message(ERROR_MESSAGE, "FE_element_field_template::cloneForNewMesh.  Invalid new mesh");
		return 0;
	}
	FE_element_field_template *eft = new FE_element_field_template(*this);
	if (!eft)
	{
		display_message(ERROR_MESSAGE, "FE_element_field_template::cloneForNewMesh.  Failed");
		return 0;
	}
	// switch ownership to newMesh
	this->mesh->removeElementfieldtemplate(eft);
	eft->mesh = newMesh;
	if (CMZN_OK != newMesh->addElementfieldtemplate(eft))
	{
		display_message(ERROR_MESSAGE, "FE_element_field_template::cloneForNewMesh.  Failed to add to mesh EFT list");
		return 0;
	}
	return eft;
}

bool FE_element_field_template::matches(const FE_element_field_template &source) const
{
	if ((this->mesh != source.mesh)
		|| (this->basis != source.basis)
		|| (this->mappingMode != source.mappingMode))
		return false;
	// Field mapping can only use a constant basis and uses no other parameters
	if (this->mappingMode != CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_FIELD)
	{
		// Note: no need to compare offset arrays since they will match if counts match
		if ((this->totalTermCount != source.totalTermCount)
			|| (this->numberOfLocalNodes != source.numberOfLocalNodes)
			|| (this->numberOfLocalScaleFactors != source.numberOfLocalScaleFactors)
			|| (this->legacyModifyThetaMode != source.legacyModifyThetaMode)
			|| (0 != memcmp(this->termCounts, source.termCounts, this->numberOfFunctions*sizeof(int))))
			return false;
		if (this->mappingMode == CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
		{
			if ((0 != memcmp(this->localNodeIndexes, source.localNodeIndexes, this->totalTermCount*sizeof(int)))
				|| (0 != memcmp(this->nodeValueLabels, source.nodeValueLabels, this->totalTermCount*sizeof(cmzn_node_value_label)))
				|| (0 != memcmp(this->nodeVersions, source.nodeVersions, this->totalTermCount*sizeof(int))))
				return false;
		}
		else if (this->mappingMode == CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_ELEMENT)
		{
			if ((this->legacyGridNumberInXi) && (source.legacyGridNumberInXi))
			{
				for (int i = 0; i < this->dimension; ++i)
				{
					if (this->legacyGridNumberInXi[i] != source.legacyGridNumberInXi[i])
						return false;
				}
			}
			else if (((this->legacyGridNumberInXi) && (!source.legacyGridNumberInXi))
				|| ((!this->legacyGridNumberInXi) && (source.legacyGridNumberInXi)))
				return false;
		}
		if (0 < this->numberOfLocalScaleFactors)
		{
			if ((this->totalLocalScaleFactorIndexes != source.totalLocalScaleFactorIndexes)
				|| (0 != memcmp(this->scaleFactorTypes, source.scaleFactorTypes, this->numberOfLocalScaleFactors*sizeof(cmzn_elementfieldtemplate_scale_factor_type)))
				|| (0 != memcmp(this->scaleFactorIdentifiers, source.scaleFactorIdentifiers, this->numberOfLocalScaleFactors*sizeof(int)))
				|| (0 != memcmp(this->termScaleFactorCounts, source.termScaleFactorCounts, this->totalTermCount*sizeof(int)))
				|| (0 != memcmp(this->localScaleFactorIndexes, source.localScaleFactorIndexes, this->totalLocalScaleFactorIndexes*sizeof(int))))
				return false;
		}
	}
	return true;
}

int FE_element_field_template::setParameterMappingMode(cmzn_elementfieldtemplate_parameter_mapping_mode modeIn)
{
	if (this->locked ||
		(!((modeIn == CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
		|| (modeIn == CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_ELEMENT)
		|| (modeIn == CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_FIELD))))
		return CMZN_ERROR_ARGUMENT;
	if (modeIn == CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_FIELD)
	{
		// can only be constant basis
		for (int i = 0; i < this->dimension; ++i)
		{
			FE_basis_type basisType = FE_BASIS_TYPE_INVALID;
			FE_basis_get_xi_basis_type(this->basis, i, &basisType);
			if (basisType != FE_BASIS_CONSTANT)
			{
				display_message(ERROR_MESSAGE, "Elementfieldtemplate setParameterMappingMode.  "
					"PARAMETER_MAPPING_MODE_FIELD only works with constant basis");
				return CMZN_ERROR_ARGUMENT;
			}
		}
	}
	this->clearNodeMapping();
	this->clearScaling();
	delete this->legacyGridNumberInXi;
	this->legacyGridNumberInXi = 0;
	this->legacyModifyThetaMode = FE_BASIS_MODIFY_THETA_MODE_INVALID;
	// reset to single term per function
	for (int fn = 0; fn < this->numberOfFunctions; ++fn)
	{
		this->termCounts[fn] = 1;
		this->termOffsets[fn] = fn;
	}
	this->totalTermCount = this->numberOfFunctions;
	this->mappingMode = modeIn;
	if (modeIn == CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
	{
		// reset to default unscaled simple node map according to basis
		this->numberOfLocalNodes = FE_basis_get_number_of_nodes(this->basis);
		this->localNodeIndexes = new int[this->numberOfFunctions];
		this->nodeValueLabels = new cmzn_node_value_label[this->numberOfFunctions];
		this->nodeVersions = new int[this->numberOfFunctions];
		for (int fn = 0; fn < this->numberOfFunctions; ++fn)
		{
			FE_basis_get_function_node_index_and_derivative(this->basis, fn, this->localNodeIndexes[fn], this->nodeValueLabels[fn]);
			this->nodeVersions[fn] = 0;
		}
	}
	return CMZN_OK;
}

int FE_element_field_template::getHighestLocalNodeIndex() const
{
	int highestNodeIndex = -1;
	if (this->mappingMode == CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
	{
		for (int tt = 0; tt < this->totalTermCount; ++tt)
		{
			if (this->localNodeIndexes[tt] > highestNodeIndex)
				highestNodeIndex = this->localNodeIndexes[tt];
		}
	}
	return highestNodeIndex;
}

/** Set legacy grid field number in xi, for repeated linear cells, or constant.
  * Only use with PARAMETER_MAPPING_MODE_ELEMENT.
  * @param numberInXiIn  Array of mesh dimension size integers >= 0, each
  * giving the number of linear cells in the respective xi direction, or
  * 0 to indicate constant over the xi direction. Pass a zero array to clear.
  * Zero values must match a constant basis in that direction, and positive
  * values must have a LINEAR_LAGRANGE basis in that direction.
  * @return  Result OK on success, otherwise ARGUMENT error. */
int FE_element_field_template::setLegacyGridNumberInXi(const int *numberInXiIn)
{
	if (this->locked
		|| (this->mappingMode != CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_ELEMENT))
		return CMZN_ERROR_ARGUMENT;
	if (!numberInXiIn)
	{
		delete this->legacyGridNumberInXi;
		this->legacyGridNumberInXi = 0;
		return CMZN_OK;
	}
	for (int i = 0; i < this->dimension; ++i)
	{
		FE_basis_type basisType;
		if (!(FE_basis_get_xi_basis_type(this->basis, i, &basisType)
			&& (((FE_BASIS_CONSTANT == basisType) && (0 == numberInXiIn[i]))
				|| ((LINEAR_LAGRANGE == basisType) && (0 < numberInXiIn[i])))))
		{
			if (0 == numberInXiIn[i])
				display_message(INFORMATION_MESSAGE, "FE_element_field_template::setLegacyGridNumberInXi.  "
					"Number in xi[%d] = 0 only works with constant basis", i);
			else
				display_message(INFORMATION_MESSAGE, "FE_element_field_template::setLegacyGridNumberInXi.  "
					"Number in xi[%d] = %d only implemented for linear basis", i, numberInXiIn[i]);
			return CMZN_ERROR_ARGUMENT;
		}
	}
	if (!this->legacyGridNumberInXi)
	{
		this->legacyGridNumberInXi = new int[this->dimension];
		if (!this->legacyGridNumberInXi)
			return CMZN_ERROR_MEMORY;
	}
	for (int i = 0; i < this->dimension; ++i)
		this->legacyGridNumberInXi[i] = numberInXiIn[i];
	return CMZN_OK;
}

int FE_element_field_template::setLegacyModifyThetaMode(FE_basis_modify_theta_mode modifyThetaModeIn)
{
	if (this->locked
		|| ((modifyThetaModeIn != FE_BASIS_MODIFY_THETA_MODE_INVALID) &&
			(this->mappingMode != CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)))
		return CMZN_ERROR_ARGUMENT;
	this->legacyModifyThetaMode = modifyThetaModeIn;
	return CMZN_OK;
}

int FE_element_field_template::getNumberOfElementDOFs() const
{
	if (this->mappingMode == CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_ELEMENT)
	{
		if (this->legacyGridNumberInXi)
		{
			// only linear grid supported, and for line shapes
			int count = 1;
			for (int i = 0; i < this->dimension; ++i)
				count *= (this->legacyGridNumberInXi[i] + 1);
			return count;
		}
		else
			return this->getNumberOfFunctions();
	}
	return 0;
}

int FE_element_field_template::setNumberOfLocalNodes(int number)
{
	if (this->locked
		|| (CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE != this->mappingMode)
		|| (number < 1))
		return CMZN_ERROR_ARGUMENT;
	this->numberOfLocalNodes = number;
	return CMZN_OK;
}

int FE_element_field_template::setFunctionNumberOfTerms(int functionNumber, int newNumberOfTerms)
{
	if (this->locked
		|| (CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE != this->mappingMode)
		|| (functionNumber < 0) || (functionNumber >= this->numberOfFunctions)
		|| (newNumberOfTerms < 0))
		return CMZN_ERROR_ARGUMENT;
	const int oldNumberOfTerms = this->termCounts[functionNumber];
	if (newNumberOfTerms == oldNumberOfTerms)
		return CMZN_OK;

	this->termCounts[functionNumber] = newNumberOfTerms;
	const int deltaTermCount = newNumberOfTerms - oldNumberOfTerms;
	this->totalTermCount += deltaTermCount;
	for (int fn = functionNumber + 1; fn < this->numberOfFunctions; ++fn)
		this->termOffsets[fn] += deltaTermCount;

	const int termOffset = this->termOffsets[functionNumber];
	const int newTermIndex = termOffset + newNumberOfTerms;
	const int oldTermIndex = termOffset + oldNumberOfTerms;

	edit_array(this->localNodeIndexes, newTermIndex, oldTermIndex, this->totalTermCount, -1); // invalid
	edit_array(this->nodeValueLabels, newTermIndex, oldTermIndex, this->totalTermCount, CMZN_NODE_VALUE_LABEL_VALUE); // default to VALUE
	edit_array(this->nodeVersions, newTermIndex, oldTermIndex, this->totalTermCount, 0); // default to first version
	if (0 < this->numberOfLocalScaleFactors)
	{
		edit_array(this->termScaleFactorCounts, newTermIndex, oldTermIndex, this->totalTermCount, 0); // unscaled
		// no scaling is added if number of terms grows
		if (deltaTermCount < 0)
		{
			// if number of terms reduces then scale factor indexes may be reduced
			if ((newTermIndex < this->totalTermCount) &&
				(this->termLocalScaleFactorIndexesOffsets[newTermIndex] <
					this->termLocalScaleFactorIndexesOffsets[oldTermIndex]))
			{
				edit_array(this->localScaleFactorIndexes,
					this->termLocalScaleFactorIndexesOffsets[newTermIndex],
					this->termLocalScaleFactorIndexesOffsets[oldTermIndex],
					this->totalLocalScaleFactorIndexes, 0); // never grows here since new terms have no scaling
			}
		}
		else
		{
			// use edit_array to grow array but recalc offsets from cumulative scale factor counts below
			edit_array(this->termLocalScaleFactorIndexesOffsets, newTermIndex, oldTermIndex, this->totalTermCount, 0);
		}
		int termLocalScaleFactorIndexesOffset = 0;
		for (int tt = 0; tt < this->totalTermCount; ++tt)
		{
			this->termLocalScaleFactorIndexesOffsets[tt] = termLocalScaleFactorIndexesOffset;
			termLocalScaleFactorIndexesOffset += this->termScaleFactorCounts[tt];
		}
		this->totalLocalScaleFactorIndexes = termLocalScaleFactorIndexesOffset;
	}
	return CMZN_OK;
}

int FE_element_field_template::getTermLocalNodeIndex(int functionNumber, int term) const
{
	if ((CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE == this->mappingMode)
		&& this->validTerm(functionNumber, term))
		return this->localNodeIndexes[this->termOffsets[functionNumber] + term];
	return -1;
}

cmzn_node_value_label FE_element_field_template::getTermNodeValueLabel(int functionNumber, int term) const
{
	if ((CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE == this->mappingMode)
		&& this->validTerm(functionNumber, term))
		return this->nodeValueLabels[this->termOffsets[functionNumber] + term];
	return CMZN_NODE_VALUE_LABEL_INVALID;
}

int FE_element_field_template::getTermNodeVersion(int functionNumber, int term) const
{
	if ((CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE == this->mappingMode)
		&& this->validTerm(functionNumber, term))
		return this->nodeVersions[this->termOffsets[functionNumber] + term];
	return -1;
}

int FE_element_field_template::getTermNodeLegacyIndex(int functionNumber, int term) const
{
	if ((CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE == this->mappingMode)
		&& this->validTerm(functionNumber, term)
		&& (this->nodeValueLabels[this->termOffsets[functionNumber] + term] == CMZN_NODE_VALUE_LABEL_INVALID))
		return this->nodeVersions[this->termOffsets[functionNumber] + term];
	return -1;
}

int FE_element_field_template::setTermNodeParameter(int functionNumber,
	int term, int localNodeIndex, cmzn_node_value_label valueLabel, int version)
{
	if (this->locked
		|| (CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE != this->mappingMode)
		|| (!this->validTerm(functionNumber, term))
		|| (localNodeIndex < 0) || (localNodeIndex >= this->numberOfLocalNodes)
		|| (!valid_node_value_label(valueLabel))
		|| (version < 0))
		return CMZN_ERROR_ARGUMENT;
	const int termOffset = this->termOffsets[functionNumber] + term;
	this->localNodeIndexes[termOffset] = localNodeIndex;
	this->nodeValueLabels[termOffset] = valueLabel;
	this->nodeVersions[termOffset] = version;
	return CMZN_OK;
}

int FE_element_field_template::setTermNodeParameterLegacyIndex(int functionNumber, int term, int localNodeIndex, int nodeDOFIndex)
{
	if (this->locked
		|| (CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE != this->mappingMode)
		|| (!this->validTerm(functionNumber, term))
		|| (localNodeIndex < 0) || (localNodeIndex >= this->numberOfLocalNodes)
		|| (nodeDOFIndex < 0))
		return CMZN_ERROR_ARGUMENT;
	const int termOffset = this->termOffsets[functionNumber] + term;
	this->localNodeIndexes[termOffset] = localNodeIndex;
	// legacy state uses invalid node value label, and stores DOF index in version
	this->nodeValueLabels[termOffset] = CMZN_NODE_VALUE_LABEL_INVALID;
	this->nodeVersions[termOffset] = nodeDOFIndex;
	return CMZN_OK;
}

bool FE_element_field_template::hasNodeParameterLegacyIndex() const
{
	if (this->mappingMode == CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
	{
		for (int tt = 0; tt < this->totalTermCount; ++tt)
		{
			// invalid node label denotes legacy case where version stores a node DOF index
			if (this->nodeValueLabels[tt] == CMZN_NODE_VALUE_LABEL_INVALID)
				return true;
		}
	}
	return false;
}

int FE_element_field_template::setNumberOfLocalScaleFactors(int number)
{
	// Future note: have to change validate code if allow scale factors with other mapping modes!
	if (this->locked
		|| (CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE != this->mappingMode)
		|| (number < 0))
		return CMZN_ERROR_ARGUMENT;
	if (number > this->numberOfLocalScaleFactors)
	{
		cmzn_elementfieldtemplate_scale_factor_type *oldScaleFactorTypes = this->scaleFactorTypes;
		int *oldScaleFactorIdentifiers = this->scaleFactorIdentifiers;
		this->scaleFactorTypes = new cmzn_elementfieldtemplate_scale_factor_type[number];
		this->scaleFactorIdentifiers = new int[number];
		// copy existing scale factor types and versions
		for (int sf = 0; sf < this->numberOfLocalScaleFactors; ++sf)
		{
			this->scaleFactorTypes[sf] = oldScaleFactorTypes[sf];
			this->scaleFactorIdentifiers[sf] = oldScaleFactorIdentifiers[sf];
		}
		// give new scale factors default type and version
		for (int sf = this->numberOfLocalScaleFactors; sf < number; ++sf)
		{
			this->scaleFactorTypes[sf] = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_GENERAL;
			this->scaleFactorIdentifiers[sf] = 0; // unique to element and EFT
		}
		if (!this->localScaleFactorIndexes)
		{
			// must ensure these arrays are allocated and filled with defaults
			this->totalLocalScaleFactorIndexes = 0;
			this->localScaleFactorIndexes = new int[this->totalLocalScaleFactorIndexes];
			this->termScaleFactorCounts = new int[this->totalTermCount];
			this->termLocalScaleFactorIndexesOffsets = new int[this->totalTermCount];
			for (int tt = 0; tt < this->totalTermCount; ++tt)
				this->termScaleFactorCounts[tt] = this->termLocalScaleFactorIndexesOffsets[tt] = 0;
		}
	}
	this->numberOfLocalScaleFactors = number;
	return CMZN_OK;
}

int FE_element_field_template::setScaleFactorType(int localScaleFactorIndex, cmzn_elementfieldtemplate_scale_factor_type type)
{
	if ((localScaleFactorIndex < 0) || (localScaleFactorIndex >= this->numberOfLocalScaleFactors)
		|| (type <= CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_INVALID))
		return CMZN_ERROR_ARGUMENT;
	this->scaleFactorTypes[localScaleFactorIndex] = type;
	return CMZN_OK;
}

int FE_element_field_template::setScaleFactorIdentifier(int localIndex, int identifier)
{
	if ((localIndex < 0) || (localIndex >= this->numberOfLocalScaleFactors)
		|| (identifier < 0))
		return CMZN_ERROR_ARGUMENT;
	this->scaleFactorIdentifiers[localIndex] = identifier;
	return CMZN_OK;
}

int FE_element_field_template::getTermScalingCount(int functionNumber, int term) const
{
	if ((CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE != this->mappingMode)
		|| (0 == this->numberOfLocalScaleFactors)
		|| (!this->validTerm(functionNumber, term)))
		return 0;
	const int termOffset = this->termOffsets[functionNumber] + term;
	return this->termScaleFactorCounts[termOffset];
}

int FE_element_field_template::getTermScaleFactorIndex(int functionNumber, int term, int termScaleFactorIndex) const
{
	if ((CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE != this->mappingMode)
		|| (0 == this->numberOfLocalScaleFactors)
		|| (!this->validTerm(functionNumber, term))
		|| (termScaleFactorIndex < 0))
		return -1;
	const int termOffset = this->termOffsets[functionNumber] + term;
	const int termScaleFactorCount = this->termScaleFactorCounts[termOffset];
	if (termScaleFactorIndex >= termScaleFactorCount)
		return -1;
	const int termLocalScaleFactorIndexesOffset = this->termLocalScaleFactorIndexesOffsets[termOffset];
	return this->localScaleFactorIndexes[termLocalScaleFactorIndexesOffset + termScaleFactorIndex];
}

int FE_element_field_template::getTermScaling(int functionNumber, int term, int indexesCount, int *indexesOut, int startIndex) const
{
	if ((CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE != this->mappingMode)
		|| (0 == this->numberOfLocalScaleFactors)
		|| (!this->validTerm(functionNumber, term))
		|| (indexesCount < 0) || ((0 < indexesCount) && (!indexesOut)))
		return -1;
	const int termOffset = this->termOffsets[functionNumber] + term;
	const int termScaleFactorCount = this->termScaleFactorCounts[termOffset];
	const int termLocalScaleFactorIndexesOffset = this->termLocalScaleFactorIndexesOffsets[termOffset];
	const int copyCount = (indexesCount < termScaleFactorCount) ? indexesCount : termScaleFactorCount;
	for (int i = 0; i < copyCount; ++i)
		indexesOut[i] = this->localScaleFactorIndexes[termLocalScaleFactorIndexesOffset + i] + startIndex;
	return termScaleFactorCount;
}

int FE_element_field_template::setTermScaling(int functionNumber, int term, int indexesCount, const int *indexesIn, int startIndex)
{
	if (this->locked
		|| (CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE != this->mappingMode)
		|| (0 == this->numberOfLocalScaleFactors)
		|| (!this->validTerm(functionNumber, term))
		|| (indexesCount < 0) || ((0 < indexesCount) && (!indexesIn)))
		return CMZN_ERROR_ARGUMENT;
	for (int i = 0; i < indexesCount; ++i)
		if ((indexesIn[i] < startIndex) || (indexesIn[i] >= (this->numberOfLocalScaleFactors + startIndex)))
			return CMZN_ERROR_ARGUMENT;
	const int termOffset = this->termOffsets[functionNumber] + term;
	const int oldTermScaleFactorCount = this->termScaleFactorCounts[termOffset];
	const int termLocalScaleFactorIndexesOffset = this->termLocalScaleFactorIndexesOffsets[termOffset];
	if (oldTermScaleFactorCount != indexesCount)
	{
		this->termScaleFactorCounts[termOffset] = indexesCount;
		const int deltaScaleFactorCount = (indexesCount - oldTermScaleFactorCount);
		this->totalLocalScaleFactorIndexes += deltaScaleFactorCount;
		edit_array(this->localScaleFactorIndexes,
			termLocalScaleFactorIndexesOffset + indexesCount,
			termLocalScaleFactorIndexesOffset + oldTermScaleFactorCount,
			this->totalLocalScaleFactorIndexes, 0); // new indexes are copied below
		// correct subsequent offsets
		for (int tt = termOffset + 1; tt < this->totalTermCount; ++tt)
			this->termLocalScaleFactorIndexesOffsets[tt] += deltaScaleFactorCount;
	}
	for (int i = 0; i < indexesCount; ++i)
		this->localScaleFactorIndexes[termLocalScaleFactorIndexesOffset + i] = indexesIn[i] - startIndex;
	return CMZN_OK;
}

int FE_element_field_template::sortNodeIndexes(std::vector<int>& extNodeIndexes)
{
	const int extNodeIndexCount = static_cast<int>(extNodeIndexes.size());
	if (this->locked
		|| (CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE != this->mappingMode)
		|| (extNodeIndexCount > this->numberOfLocalNodes))
		return CMZN_ERROR_ARGUMENT;
	int lastExtNodeIndex = -1;
	int newNodeIndex = 0;
	for (int tt = 0; tt < this->totalTermCount; ++tt)
	{
		const int nodeIndex = this->localNodeIndexes[tt];
		if ((nodeIndex < 0) || (nodeIndex >= extNodeIndexCount))
			return CMZN_ERROR_ARGUMENT;
		const int extNodeIndex = extNodeIndexes[nodeIndex];
		if (extNodeIndex != lastExtNodeIndex)
		{
			newNodeIndex = 0;
			for (int e = 0; e < extNodeIndexCount; ++e)
				if (extNodeIndexes[e] < extNodeIndex)
					++newNodeIndex;
			lastExtNodeIndex = extNodeIndex;
		}
		this->localNodeIndexes[tt] = newNodeIndex;
	}
	std::sort(extNodeIndexes.begin(), extNodeIndexes.end());
	return CMZN_OK;
}

int FE_element_field_template::getParameterTermCount(int parameterIndex) const
{
	if ((!this->locked) || (parameterIndex < 0) || (this->parameterCount <= parameterIndex))
		return 0;
	if (this->parameterTermCounts)
		return this->parameterTermCounts[parameterIndex];
	return 1;
}

int FE_element_field_template::getParameterTermFunctionAndTerm(int parameterIndex, int parameterTerm, int &term) const
{
	if ((!this->locked) || (parameterIndex < 0) || (this->parameterCount <= parameterIndex)
		|| (parameterIndex < 0))
		return -1;
	if (this->parameterTermCounts)
	{
		if (parameterTerm >= this->parameterTermCounts[parameterIndex])
			return -1;
		const int offset = this->parameterTermOffsets[parameterIndex] + 2*parameterTerm;
		term = this->parameterFunctionTerms[offset + 1];
		return this->parameterFunctionTerms[offset];
	}
	if (parameterTerm > 0)
		return -1;
	term = 0;
	return parameterIndex;
}

bool FE_element_field_template::validateAndLock()
{
	if (this->locked)
		return true; // if locked, it's already validated
	// following lists all the things that are invalid, but only once
	bool valid = true;
	if (this->mappingMode == CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
	{
		bool localNodesValid = true;
		bool nodeValueLabelsValid = true;
		bool nodeVersionsValid = true;
		for (int tt = 0; tt < this->totalTermCount; ++tt)
		{
			if (localNodesValid &&
				((this->localNodeIndexes[tt] < 0) || (this->localNodeIndexes[tt] >= this->numberOfLocalNodes)))
			{
				display_message(ERROR_MESSAGE, "Elementfieldtemplate validate:  Local node index out of range");
				valid = localNodesValid = false;
			}
			// invalid node label denotes legacy case where version stores a node DOF index
			if (nodeValueLabelsValid && (this->nodeValueLabels[tt] != CMZN_NODE_VALUE_LABEL_INVALID) &&
				((this->nodeValueLabels[tt] < CMZN_NODE_VALUE_LABEL_VALUE)
					|| (this->nodeValueLabels[tt] > CMZN_NODE_VALUE_LABEL_D3_DS1DS2DS3)))
			{
				display_message(ERROR_MESSAGE, "Elementfieldtemplate validate:  Node value label is invalid");
				valid = nodeValueLabelsValid = false;
			}
			if (nodeVersionsValid && (this->nodeVersions[tt] < 0))
			{
				display_message(ERROR_MESSAGE, "Elementfieldtemplate validate:  Node version is invalid");
				valid = nodeValueLabelsValid = false;
			}
		}
		if (0 < this->numberOfLocalScaleFactors)
		{
			std::vector<int> scaleFactorUseCounts(this->numberOfLocalScaleFactors, 0);
			bool scaleFactorIndexesValid = true;
			for (int si = 0; si < this->totalLocalScaleFactorIndexes; ++si)
			{
				const int localScaleFactorIndex = this->localScaleFactorIndexes[si];
				if ((0 <= localScaleFactorIndex) && (localScaleFactorIndex < this->numberOfLocalScaleFactors))
				{
					++(scaleFactorUseCounts[localScaleFactorIndex]);
				}
				else if (scaleFactorIndexesValid)
				{
					display_message(ERROR_MESSAGE, "Elementfieldtemplate validate:  Term local scale factor index out of range");
					valid = scaleFactorIndexesValid = false;  // so only warn once
				}
			}
			bool hasNodeScaleFactors = false;
			bool scaleFactorIdentifiersValidElement = true;
			bool scaleFactorIdentifiersValidOther = true;
			for (int s = 0; s < this->numberOfLocalScaleFactors; ++s)
			{
				if (scaleFactorUseCounts[s] == 0)
				{
					display_message(ERROR_MESSAGE, "Elementfieldtemplate validate:  Local scale factor %d is not used", s + 1);
					valid = false;
				}
				if (isScaleFactorTypeElement(this->scaleFactorTypes[s]))
				{
					if (scaleFactorIdentifiersValidElement && (this->scaleFactorIdentifiers[s] != 0))
					{
						display_message(ERROR_MESSAGE, "Elementfieldtemplate validate:  ELEMENT type scale factor identifier must be 0");
						valid = scaleFactorIdentifiersValidElement = false;
					}
				}
				else
				{
					if (scaleFactorIdentifiersValidOther && (this->scaleFactorIdentifiers[s] < 1))
					{
						display_message(ERROR_MESSAGE, "Elementfieldtemplate validate:  NODE or GLOBAL type scale factor identifier must be > 0");
						valid = scaleFactorIdentifiersValidOther = false;
					}
					if (isScaleFactorTypeNode(this->scaleFactorTypes[s]))
					{
						hasNodeScaleFactors = true;
					}
				}
			}
			if (hasNodeScaleFactors)
			{
				this->scaleFactorLocalNodeIndexes = new int[this->numberOfLocalScaleFactors];
				if (!this->scaleFactorLocalNodeIndexes)
				{
					display_message(ERROR_MESSAGE, "Elementfieldtemplate validate:  Failed to allocate scale factor local node indexes");
					valid = false;
				}
				else
				{
					for (int s = 0; s < this->numberOfLocalScaleFactors; ++s)
					{
						this->scaleFactorLocalNodeIndexes[s] = -1;
					}
					for (int tt = 0; tt < this->totalTermCount; ++tt)
					{
						const int localNodeIndex = this->localNodeIndexes[tt];
						const int scaleFactorCount = termScaleFactorCounts[tt];
						const int localScaleFactorIndexesOffset = this->termLocalScaleFactorIndexesOffsets[tt];
						for (int si = localScaleFactorIndexesOffset; si < localScaleFactorIndexesOffset + scaleFactorCount; ++si)
						{
							const int localScaleFactorIndex = this->localScaleFactorIndexes[si];
							if (isScaleFactorTypeNode(this->scaleFactorTypes[localScaleFactorIndex]))
							{
								if (this->scaleFactorLocalNodeIndexes[localScaleFactorIndex] < 0)
								{
									this->scaleFactorLocalNodeIndexes[localScaleFactorIndex] = localNodeIndex;
								}
								else if (this->scaleFactorLocalNodeIndexes[localScaleFactorIndex] != localNodeIndex)
								{
									display_message(ERROR_MESSAGE, "Elementfieldtemplate validate:  "
										"Scale factor %d is node type, but scales parameters from more than one local node",
										localScaleFactorIndex + 1);
									valid = false;
								}
							}
						}
					}
					for (int s = 0; s < this->numberOfLocalScaleFactors; ++s)
					{
						if (isScaleFactorTypeNode(this->scaleFactorTypes[s]) && (this->scaleFactorLocalNodeIndexes[s] < 0))
						{
							display_message(ERROR_MESSAGE, "Elementfieldtemplate validate:  "
								"Scale factor %d is node type, but is not scaling a parameter from a local node", s + 1);
							valid = false;
						}
					}
				}
			}
		}
	}
	if (valid)
	{
		// calculate parameter maps
		this->parameterCount = this->numberOfFunctions;  // default
		this->parameterFunctionTermsSize = 0;
		if (CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE == this->mappingMode)
		{
			// number of parameters cannot exceed totalTermCount, so allocate to this and possibly waste a little
			this->parameterTermCounts = new int[this->totalTermCount];
			this->parameterTermOffsets = new int[this->totalTermCount];
			this->parameterFunctionTerms = new int[2*this->totalTermCount];  // 2x for function, term
			if (!((this->parameterTermCounts) && (this->parameterTermOffsets) && (this->parameterFunctionTerms)))
			{
				display_message(ERROR_MESSAGE, "Elementfieldtemplate validate:  Failed to allocate parameter maps");
				valid = false;
			}
			else
			{
				bool oneTermPerFunction = true;  // until proven false
				this->parameterCount = 0;
				for (int f = 0; f < this->numberOfFunctions; ++f)
				{
					if (oneTermPerFunction && (this->termCounts[f] != 1))
						oneTermPerFunction = false;
					for (int t = 0; t < this->termCounts[f]; ++t)
					{
						const int ft = this->termOffsets[f] + t;
						int p = 0;
						for (; p < this->parameterCount; ++p)
						{
							const int pftOffset = this->parameterTermOffsets[p];  // first parameter term only
							const int ftExisting = this->termOffsets[this->parameterFunctionTerms[pftOffset]] + this->parameterFunctionTerms[pftOffset + 1];
							if ((this->localNodeIndexes[ft] == this->localNodeIndexes[ftExisting])
								&& (this->nodeValueLabels[ft] == this->nodeValueLabels[ftExisting])
								&& (this->nodeVersions[ft] == this->nodeVersions[ftExisting]))
							{
								break;
							}
						}
						int pftOffset;
						if (p < this->parameterCount)
						{
							// insert new term for existing parameter p: offset later parameter function terms
							for (int np = p + 1; np < this->parameterCount; ++np)
								this->parameterTermOffsets[np] += 2;
							pftOffset = this->parameterTermOffsets[p] + 2*parameterTermCounts[p];
							for (int os = this->parameterFunctionTermsSize - 1; os >= pftOffset; --os)
								this->parameterFunctionTerms[os + 2] = this->parameterFunctionTerms[os];
							++(this->parameterTermCounts[p]);
						}
						else
						{
							// add new parameter with one term
							this->parameterTermCounts[this->parameterCount] = 1;
							this->parameterTermOffsets[this->parameterCount] = this->parameterFunctionTermsSize;
							++(this->parameterCount);
							pftOffset = this->parameterFunctionTermsSize;
						}
						this->parameterFunctionTerms[pftOffset] = f;
						this->parameterFunctionTerms[pftOffset + 1] = t;
						this->parameterFunctionTermsSize += 2;
					}
				}
				if (oneTermPerFunction && (this->parameterCount == this->numberOfFunctions))
				{
					// can clear maps and use default behaviour
					this->clearParameterMaps();
					this->parameterCount = this->numberOfFunctions;
				}
			}
		}
	}
	if (valid)
	{
		this->locked = true;
	}
	else
	{
		// free these as only exist if validated and locked since not dynamically resizing
		delete[] this->scaleFactorLocalNodeIndexes;
		this->scaleFactorLocalNodeIndexes = 0;
		this->clearParameterMaps();
	}
	return valid;
}

bool cmzn_elementfieldtemplate::findMergedInMesh()
{
	if (!this->impl->getMesh())
	{
		return false;
	}
	FE_element_field_template *newEft = this->impl->getMesh()->findMergedElementfieldtemplate(this->impl);
	if (!newEft)
	{
		return false;
	}
	FE_element_field_template::deaccess(this->impl);
	this->impl = newEft;
	return true;
}

cmzn_elementbasis *cmzn_elementfieldtemplate::getElementbasis() const
{
	return cmzn_elementbasis::create(this->getMesh()->get_FE_region(), this->getBasis());
}

cmzn_elementfieldtemplate_id cmzn_elementfieldtemplate_access(
	cmzn_elementfieldtemplate_id elementfieldtemplate)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->access();
	return 0;
}

int cmzn_elementfieldtemplate_destroy(
	cmzn_elementfieldtemplate_id *elementfieldtemplate_address)
{
	if (elementfieldtemplate_address)
		return cmzn_elementfieldtemplate::deaccess(*elementfieldtemplate_address);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_elementbasis_id cmzn_elementfieldtemplate_get_elementbasis(
	cmzn_elementfieldtemplate_id elementfieldtemplate)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->getElementbasis();
	return 0;
}

enum cmzn_elementfieldtemplate_scale_factor_type
	cmzn_elementfieldtemplate_get_scale_factor_type(
		cmzn_elementfieldtemplate_id elementfieldtemplate, int localIndex)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->getScaleFactorType(localIndex);
	return CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_INVALID;
}

int cmzn_elementfieldtemplate_set_scale_factor_type(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int localIndex,
	enum cmzn_elementfieldtemplate_scale_factor_type type)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->setScaleFactorType(localIndex, type);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_elementfieldtemplate_get_scale_factor_identifier(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int localScaleFactorIndex)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->getScaleFactorIdentifier(localScaleFactorIndex);
	return -1;
}

int cmzn_elementfieldtemplate_set_scale_factor_identifier(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int localIndex,
	int identifier)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->setScaleFactorIdentifier(localIndex, identifier);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_elementfieldtemplate_get_function_number_of_terms(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->getFunctionNumberOfTerms(functionNumber);
	return -1;
}

int cmzn_elementfieldtemplate_set_function_number_of_terms(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber,
	int newNumberOfTerms)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->setFunctionNumberOfTerms(functionNumber, newNumberOfTerms);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_elementfieldtemplate_get_number_of_functions(
	cmzn_elementfieldtemplate_id elementfieldtemplate)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->getNumberOfFunctions();
	return 0;
}

int cmzn_elementfieldtemplate_get_number_of_local_nodes(
	cmzn_elementfieldtemplate_id elementfieldtemplate)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->getNumberOfLocalNodes();
	return 0;
}

int cmzn_elementfieldtemplate_set_number_of_local_nodes(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int number)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->setNumberOfLocalNodes(number);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_elementfieldtemplate_get_number_of_local_scale_factors(
	cmzn_elementfieldtemplate_id elementfieldtemplate)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->getNumberOfLocalScaleFactors();
	return 0;
}

int cmzn_elementfieldtemplate_set_number_of_local_scale_factors(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int number)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->setNumberOfLocalScaleFactors(number);
	return CMZN_ERROR_ARGUMENT;
}

enum cmzn_elementfieldtemplate_parameter_mapping_mode
	cmzn_elementfieldtemplate_get_parameter_mapping_mode(
		cmzn_elementfieldtemplate_id elementfieldtemplate)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->getParameterMappingMode();
	return CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_INVALID;
}

int cmzn_elementfieldtemplate_set_parameter_mapping_mode(
	cmzn_elementfieldtemplate_id elementfieldtemplate,
	enum cmzn_elementfieldtemplate_parameter_mapping_mode mode)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->setParameterMappingMode(mode);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_elementfieldtemplate_get_term_local_node_index(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber,
	int term)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->getTermLocalNodeIndex(functionNumber, term);
	return 0;
}

enum cmzn_node_value_label cmzn_elementfieldtemplate_get_term_node_value_label(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber,
	int term)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->getTermNodeValueLabel(functionNumber, term);
	return CMZN_NODE_VALUE_LABEL_INVALID;
}

int cmzn_elementfieldtemplate_get_term_node_version(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber,
	int term)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->getTermNodeVersion(functionNumber, term);
	return 0;
}

int cmzn_elementfieldtemplate_set_term_node_parameter(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber,
	int term, int localNodeIndex, cmzn_node_value_label valueLabel, int version)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->setTermNodeParameter(functionNumber, term,
			localNodeIndex, valueLabel, version);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_elementfieldtemplate_get_term_scaling(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber,
	int term, int indexesCount, int *indexesOut)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->getTermScaling(functionNumber, term, indexesCount, indexesOut);
	return -1;
}

int cmzn_elementfieldtemplate_set_term_scaling(
	cmzn_elementfieldtemplate_id elementfieldtemplate, int functionNumber,
	int term, int indexesCount, const int *indexesIn)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->setTermScaling(functionNumber, term, indexesCount, indexesIn);
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_elementfieldtemplate_validate(
	cmzn_elementfieldtemplate_id elementfieldtemplate)
{
	if (elementfieldtemplate)
		return elementfieldtemplate->validateAndLock();
	return false;
}

cmzn_elementfieldtemplate_id cmzn_mesh_create_elementfieldtemplate(
	cmzn_mesh_id mesh, cmzn_elementbasis_id elementbasis)
{
	if (mesh && elementbasis)
	{
		FE_basis *basis = cmzn_elementbasis_get_FE_basis(elementbasis);
		cmzn_elementfieldtemplate_id eft = cmzn_elementfieldtemplate::create(cmzn_mesh_get_FE_mesh_internal(mesh), basis);
		return eft;
	}
	return 0;
}


