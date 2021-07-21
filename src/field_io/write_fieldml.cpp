/**
 * FILE : write_fieldml.cpp
 * 
 * FieldML 0.5 model writer implementation.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include "opencmiss/zinc/core.h"
#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/fieldfiniteelement.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/node.h"
#include "opencmiss/zinc/region.h"
#include "opencmiss/zinc/status.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_finite_element.h"
#include "datastore/labels.hpp"
#include "field_io/fieldml_common.hpp"
#include "field_io/write_fieldml.hpp"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_basis.hpp"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_private.h"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_region_private.h"
#include "general/debug.h"
#include "general/message.h"
#include "general/mystring.h"
#include "general/refcounted.hpp"
#include "general/refhandle.hpp"
#include "mesh/cmiss_node_private.hpp"
#include "region/cmiss_region.hpp"
#include "FieldmlIoApi.h"

namespace {

	class FE_mesh_element_shape_type_parameter_generator
	{
		const FE_mesh *mesh;
		const FmlObjectHandle fmlElementsArgument;
		DsLabelIterator *elementLabelIterator;
		DsLabelIndex elementIndex;
		const int recordIndexSize; // never used as always dense
		const int recordSize;
		const int elementIdentifier; // never used as always dense
		int value; // store in member so can return values address
		int offset; // offset incremented for each record

	public:

		FE_mesh_element_shape_type_parameter_generator(const FE_mesh *meshIn, FmlObjectHandle fmlElementsArgumentIn) :
			mesh(meshIn),
			fmlElementsArgument(fmlElementsArgumentIn),
			elementLabelIterator(this->mesh->getLabels().createLabelIterator()),
			elementIndex(DS_LABEL_INDEX_INVALID),
			recordIndexSize(1),
			recordSize(1),
			elementIdentifier(-1),
			offset(-1)
		{
		}

		~FE_mesh_element_shape_type_parameter_generator()
		{
			cmzn::Deaccess(this->elementLabelIterator);
		}

		bool isDense() const
		{
			return true;
		}

		const FmlObjectHandle *getDenseArguments(int &count) const
		{
			count = 1;
			return &fmlElementsArgument;
		}

		const FmlObjectHandle *getSparseArguments(int &count) const
		{
			count = 0;
			return 0;
		}

		int getRecordCount() const
		{
			return this->mesh->getSize();
		}

		const int *getRecordIndexSizes() const
		{
			return &this->recordIndexSize;
		}

		const int *getDenseRecordSizes() const
		{
			return &this->recordSize;
		}

		/** Advance to next record or first if starting.
		  * @return  True if there is a record, false if not */
		bool nextRecord()
		{
			this->elementIndex = elementLabelIterator->nextIndex();
			if (this->elementIndex < 0)
			{
				display_message(ERROR_MESSAGE, "FieldML Writer:  Iterated beyond last element for shape type");
				return false;
			}
			++this->offset;
			this->value = this->mesh->getElementShapeFacesIndex(this->elementIndex) + 1;
			return true;
		}

		const int *getRecordIndexes() const
		{
			return &this->elementIdentifier;
		}

		const int *getRecordValues() const
		{
			return &this->value;
		}

		const int *getRecordOffsets() const
		{
			return &this->offset;
		}
	};

	class FE_mesh_local_to_global_nodes_parameter_generator
	{
		const FE_mesh_element_field_template_data *meshEFTData;
		const FmlObjectHandle fmlElementsArgument;
		const FmlObjectHandle fmlEftNodesArgument;
		FmlObjectHandle fmlDenseArguments[2]; // only used to return values when dense
		const FE_element_field_template *eft;
		const int localNodeCount;
		const FE_mesh *mesh;
		const FE_nodeset *nodeset;
		const bool isMapDense;
		const int recordCount;
		DsLabelIterator *elementLabelIterator;
		const int recordIndexSize;
		int recordSizes[2];
		int elementIdentifier;
		int *nodeIdentifiers;
		mutable int offsets[2]; // offset incremented for each record

	public:

		FE_mesh_local_to_global_nodes_parameter_generator(const FE_mesh_element_field_template_data *meshEFTDataIn,
				FmlObjectHandle fmlElementsArgumentIn, FmlObjectHandle fmlEftNodesArgumentIn) :
			meshEFTData(meshEFTDataIn),
			fmlElementsArgument(fmlElementsArgumentIn),
			fmlEftNodesArgument(fmlEftNodesArgumentIn),
			eft(this->meshEFTData->getElementfieldtemplate()),
			localNodeCount(this->eft->getNumberOfLocalNodes()),
			mesh(this->eft->getMesh()),
			nodeset(this->mesh->getNodeset()),
			isMapDense(meshEFTData->localToGlobalNodesIsDense()),
			recordCount(meshEFTData->getElementLocalToGlobalNodeMapCount()),
			elementLabelIterator(this->mesh->getLabels().createLabelIterator()),
			recordIndexSize(1),
			elementIdentifier(-1),
			nodeIdentifiers(new int[this->localNodeCount])
		{
			this->recordSizes[0] = 1;
			this->recordSizes[1] = this->localNodeCount;
			this->offsets[0] = -1;
			this->offsets[1] = 0;
			this->fmlDenseArguments[0] = this->fmlElementsArgument;
			this->fmlDenseArguments[1] = this->fmlEftNodesArgument;
		}

		~FE_mesh_local_to_global_nodes_parameter_generator()
		{
			cmzn::Deaccess(this->elementLabelIterator);
			delete[] nodeIdentifiers;
		}

		bool isDense() const
		{
			return this->isMapDense;
		}

		const FmlObjectHandle *getDenseArguments(int &count) const
		{
			if (this->isMapDense)
			{
				count = 2;
				return this->fmlDenseArguments;
			}
			count = 1;
			return &this->fmlEftNodesArgument;
		}

		const FmlObjectHandle *getSparseArguments(int &count) const
		{
			if (this->isMapDense)
			{
				count = 0;
				return 0;
			}
			count = 1;
			return &this->fmlElementsArgument;
		}

		int getRecordCount() const
		{
			return this->recordCount;
		}

		const int *getRecordIndexSizes() const
		{
			return &this->recordIndexSize;
		}

		const int *getDenseRecordSizes() const
		{
			if (this->isMapDense)
				return this->recordSizes;
			return this->recordSizes + 1;
		}

		/** Advance to next record or first if starting.
		  * @return  True if there is a record, false if not */
		bool nextRecord()
		{
			const DsLabelIndex *nodeIndexes = 0;
			DsLabelIndex elementIndex;
			do
			{
				elementIndex = elementLabelIterator->nextIndex();
				if (elementIndex < 0)
				{
					display_message(ERROR_MESSAGE, "FieldML Writer:  Iterated beyond last element for local to global node map");
					return false;
				}
				nodeIndexes = this->meshEFTData->getElementNodeIndexes(elementIndex);
			} while (nodeIndexes == 0);
			for (int n = 0; n < this->localNodeCount; ++n)
				this->nodeIdentifiers[n] = this->nodeset->getNodeIdentifier(nodeIndexes[n]);
			if (!this->isMapDense)
				this->elementIdentifier = this->mesh->getElementIdentifier(elementIndex);
			++(this->offsets[0]);
			return true;
		}

		const int *getRecordIndexes() const
		{
			return &this->elementIdentifier;
		}

		const int *getRecordValues() const
		{
			return this->nodeIdentifiers;
		}

		const int *getRecordOffsets() const
		{
			return this->offsets;
		}
	};

	class FE_mft_eft_map_parameter_generator
	{
		const FE_mesh_field_template *mft;
		const FE_mesh *mesh;
		const FmlObjectHandle fmlElementsArgument;
		const bool isMapDense; // passed in since pre-determined in FieldML Writer code
		const std::vector<int> outputEftIndexes;
		std::vector<bool> eftsUsed; // remember which efts are used, by output index - 1
		const int recordCount;
		DsLabelIterator *elementLabelIterator;
		const int recordIndexSize;
		const int recordSize;
		int elementIdentifier;
		int value; // store in member so can return values address
		mutable int offset; // offset incremented for each record

	public:

		FE_mft_eft_map_parameter_generator(const FE_mesh_field_template *mftIn,
			FmlObjectHandle fmlElementsArgumentIn, bool isMapDenseIn, int outputEftCount, const std::vector<int> outputEftIndexesIn) :
			mft(mftIn),
			mesh(this->mft->getMesh()),
			fmlElementsArgument(fmlElementsArgumentIn),
			isMapDense(isMapDenseIn),
			outputEftIndexes(outputEftIndexesIn),
			eftsUsed(outputEftCount, false),
			recordCount(mft->getElementsDefinedCount()),
			elementLabelIterator(this->mesh->getLabels().createLabelIterator()),
			recordIndexSize(1),
			recordSize(1),
			elementIdentifier(-1),
			offset(-1)
		{
		}

		~FE_mft_eft_map_parameter_generator()
		{
			cmzn::Deaccess(this->elementLabelIterator);
		}

		bool isDense() const
		{
			return this->isMapDense;
		}

		const FmlObjectHandle *getDenseArguments(int &count) const
		{
			if (this->isMapDense)
			{
				count = 1;
				return &this->fmlElementsArgument;
			}
			count = 0;
			return 0;
		}

		const FmlObjectHandle *getSparseArguments(int &count) const
		{
			if (this->isMapDense)
			{
				count = 0;
				return 0;
			}
			count = 1;
			return &this->fmlElementsArgument;
		}

		int getRecordCount() const
		{
			return this->recordCount;
		}

		const int *getRecordIndexSizes() const
		{
			return &this->recordIndexSize;
		}

		const int *getDenseRecordSizes() const
		{
			return &this->recordSize;
		}

		/** Advance to next record or first if starting.
		* @return  True if there is a record, false if not */
		bool nextRecord()
		{
			DsLabelIndex elementIndex;
			FE_mesh_field_template::EFTIndexType eftIndex;
			do
			{
				elementIndex = elementLabelIterator->nextIndex();
				if (elementIndex < 0)
				{
					display_message(ERROR_MESSAGE, "FieldML Writer:  Iterated beyond last element for mesh field template eft map");
					return false;
				}
				eftIndex = static_cast<int>(this->mft->getElementEFTIndex(elementIndex));
			} while (eftIndex < 0);
			this->value = this->outputEftIndexes[eftIndex];
			this->eftsUsed[this->value - 1] = true;
			if (!this->isMapDense)
				this->elementIdentifier = this->mesh->getElementIdentifier(elementIndex);
			++this->offset;
			return true;
		}

		const int *getRecordIndexes() const
		{
			return &this->elementIdentifier;
		}

		const int *getRecordValues() const
		{
			return &this->value;
		}

		const int *getRecordOffsets() const
		{
			return &this->offset;
		}

		bool isEftIndexUsed(int outputEftIndexes) const
		{
			return this->eftsUsed[outputEftIndexes];
		}
	};

};

namespace {

const char *derivativeNames[8] = { "value", "d_ds1", "d_ds2", "d2_ds1ds2", "d_ds3", "d2_ds1ds3", "d2_ds2ds3", "d3_ds1ds2ds3" };

}

class FieldMLWriter
{
	cmzn_region *region; // accessed
	FE_region *fe_region; // not accessed
	const char *location;
	const char *filename;
	char *regionName;
	FmlSessionHandle fmlSession;
	bool verbose;
	int libraryImportSourceIndex;
	std::map<cmzn_field_domain_type,FmlObjectHandle> fmlNodesTypes;
	std::map<cmzn_field_domain_type,FmlObjectHandle> fmlNodesParametersArguments;
	HDsLabels nodeDerivatives;
	FmlObjectHandle fmlNodeDerivativesType, fmlNodeDerivativesArgument;
	std::vector<FmlObjectHandle> fmlNodeDerivativeConstants;
	HDsLabels nodeVersions;
	FmlObjectHandle fmlNodeVersionsType, fmlNodeVersionsArgument;
	std::vector<FmlObjectHandle> fmlNodeVersionConstants;
	FmlObjectHandle fmlZeroEvaluator;
	std::vector<FmlObjectHandle> fmlMeshElementsType;
	std::vector<HDsLabels> hermiteNodeValueLabels;
	std::vector<FmlObjectHandle> fmlHermiteNodeValueLabels;
	std::map<FmlObjectHandle,FmlObjectHandle> typeArgument;

public:
	FieldMLWriter(struct cmzn_region *region, const char *locationIn, const char *filenameIn) :
		region(cmzn_region_access(region)),
		fe_region(this->region->get_FE_region()),
		location(locationIn),
		filename(filenameIn),
		fmlSession(Fieldml_Create(location, /*regionName*/"/")),
		verbose(false),
		libraryImportSourceIndex(-1),
		fmlNodeDerivativesType(FML_INVALID_OBJECT_HANDLE),
		fmlNodeDerivativesArgument(FML_INVALID_OBJECT_HANDLE),
		fmlNodeVersionsType(FML_INVALID_OBJECT_HANDLE),
		fmlNodeVersionsArgument(FML_INVALID_OBJECT_HANDLE),
		fmlZeroEvaluator(FML_INVALID_OBJECT_HANDLE),
		fmlMeshElementsType(MAXIMUM_ELEMENT_XI_DIMENSIONS + 1),
		hermiteNodeValueLabels(MAXIMUM_ELEMENT_XI_DIMENSIONS + 1),
		fmlHermiteNodeValueLabels(MAXIMUM_ELEMENT_XI_DIMENSIONS + 1)
	{
		Fieldml_SetDebug(fmlSession, /*debug*/verbose);
		for (int i = 0; i < 4; ++i)
		{
			fmlMeshElementsType[i] = FML_INVALID_OBJECT_HANDLE;
			fmlHermiteNodeValueLabels[i] = FML_INVALID_OBJECT_HANDLE;
		}
	}

	~FieldMLWriter()
	{
		Fieldml_Destroy(fmlSession);
		cmzn_region_destroy(&region);
	}

	int setMinimumNodeVersions(int minimumNodeVersions);

	int writeNodeset(cmzn_field_domain_type domainType, bool writeIfEmpty);
	int writeNodesets();

	int getHighestMeshDimension() const;

	int writeMesh(int meshDimension, bool writeIfEmpty);
	int writeMeshFields(int meshDimension);

	int writeFile(const char *pathandfilename);

private:
	FmlObjectHandle libraryImport(const char *remoteName);
	FmlObjectHandle getArgumentForType(FmlObjectHandle fmlType);
	FmlObjectHandle getBasisEvaluator(FE_basis *basis,
		FmlObjectHandle &fmlBasisParametersType, FmlObjectHandle &fmlBasisParametersArgument,
		FmlObjectHandle &fmlBasisParametersComponentType, FmlObjectHandle &fmlBasisParametersComponentArgument);
	int defineEnsembleFromLabels(FmlObjectHandle fmlEnsembleType, const DsLabels& labels);
	template <typename VALUETYPE, class PARAMETERGENERATOR>
	FmlObjectHandle writeDenseParameters(const std::string& name,
		FmlObjectHandle fmlValueType, PARAMETERGENERATOR& parameterGenerator) const;
	template <typename VALUETYPE, class PARAMETERGENERATOR>
	FmlObjectHandle writeSparseParameters(const std::string& name,
		FmlObjectHandle fmlValueType, PARAMETERGENERATOR& parameterGenerator) const;
	template <typename VALUETYPE> FmlObjectHandle defineParametersFromMap(
		DsMap<VALUETYPE>& parameterMap, FmlObjectHandle fmlValueType);

	FmlObjectHandle getZeroEvaluator();
	FmlObjectHandle writeElementfieldtemplate(const FE_element_field_template *eft, const std::string& name,
		FmlObjectHandle& fmlEftNodesArgument, FmlObjectHandle& fmlEftNodeParametersArgument,
		FmlObjectHandle& fmlEftScaleFactorIndexesArgument, FmlObjectHandle& fmlEftScaleFactorsArgument);
	FmlObjectHandle writeMeshElementEvaluator(const FE_mesh *mesh, const FE_mesh_element_field_template_data *meshEFTData, const std::string& eftName);
	FmlObjectHandle writeMeshfieldtemplate(const FE_mesh *mesh, const FE_mesh_field_template *mft,
		const std::string name, bool isDense, int mftEftCount, FmlObjectHandle fmlEftIndexes,
		int outputEftCount, const std::vector<int> outputEftIndexes,
		const std::vector<FmlObjectHandle> fmlMeshElementEvaluators);
	FmlObjectHandle writeMeshField(const FE_mesh *mesh, FE_field *field,
		const std::map<const FE_mesh_field_template *, FmlObjectHandle> fmlMftsMap);

};

FmlObjectHandle FieldMLWriter::libraryImport(const char *remoteName)
{
	FmlObjectHandle fmlImport = Fieldml_GetObjectByName(this->fmlSession, remoteName);
	if (FML_INVALID_OBJECT_HANDLE != fmlImport)
		return fmlImport;
	if (-1 == this->libraryImportSourceIndex)
	{
		this->libraryImportSourceIndex = Fieldml_AddImportSource(this->fmlSession,
			"http://www.fieldml.org/resources/xml/0.5/FieldML_Library_0.5.xml", "library");
	}
	fmlImport = Fieldml_AddImport(this->fmlSession, libraryImportSourceIndex, remoteName, remoteName);
	if (fmlImport == FML_INVALID_OBJECT_HANDLE)
		display_message(ERROR_MESSAGE, "Failed to import %s from library", remoteName);
	return fmlImport;
}

FmlObjectHandle FieldMLWriter::getArgumentForType(FmlObjectHandle fmlType)
{
	FieldmlHandleType objectType = Fieldml_GetObjectType(this->fmlSession, fmlType);
	if ((objectType != FHT_ENSEMBLE_TYPE) &&
		(objectType != FHT_CONTINUOUS_TYPE) &&
		(objectType != FHT_MESH_TYPE))
		return FML_INVALID_OBJECT_HANDLE;
	std::map<FmlObjectHandle,FmlObjectHandle>::iterator iter = this->typeArgument.find(fmlType);
	if (iter != this->typeArgument.end())
		return iter->second;
	char *objectName = Fieldml_GetObjectName(this->fmlSession, fmlType);
	if (!objectName)
		return FML_INVALID_OBJECT_HANDLE;
	std::string argumentName(objectName);
	argumentName += ".argument";
	FmlObjectHandle fmlArgument = Fieldml_CreateArgumentEvaluator(this->fmlSession, argumentName.c_str(), fmlType);
	this->typeArgument[fmlType] = fmlArgument;
	Fieldml_FreeString(objectName);
	return fmlArgument;
}

FmlObjectHandle FieldMLWriter::getBasisEvaluator(FE_basis *basis,
	FmlObjectHandle &fmlBasisParametersType, FmlObjectHandle &fmlBasisParametersArgument,
	FmlObjectHandle &fmlBasisParametersComponentType, FmlObjectHandle &fmlBasisParametersComponentArgument)
{
	int basisDimension = 0;
	FE_basis_get_dimension(basis, &basisDimension);
	enum cmzn_elementbasis_function_type functionType[MAXIMUM_ELEMENT_XI_DIMENSIONS];
	for (int i = 0; i < basisDimension; ++i)
		functionType[i] = FE_basis_get_xi_elementbasis_function_type(basis, i);
	int basisIndex = -1;
	for (int b = 0; b < numLibraryBases; b++)
	{
		if (libraryBases[b].dimension == basisDimension)
		{
			bool match = true;
			for (int i = 0; i < basisDimension; ++i)
				if (libraryBases[b].functionType[i] != functionType[i])
				{
					match = false;
					break;
				}
			if (match)
			{
				basisIndex = b;
				break;
			}
		}
	}
	if (basisIndex < 0)
	{
		char *description = FE_basis_get_description_string(basis);
		display_message(ERROR_MESSAGE, "FieldML Writer:  Does not support basis %s", description);
		DEALLOCATE(description);
		return FML_INVALID_OBJECT_HANDLE;
	}

	std::string basisEvaluatorName(libraryBases[basisIndex].fieldmlBasisEvaluatorName);
	FmlObjectHandle fmlBasisEvaluator = this->libraryImport(basisEvaluatorName.c_str());
	if (fmlBasisEvaluator == FML_INVALID_OBJECT_HANDLE)
		return FML_INVALID_OBJECT_HANDLE;
	// assumes starts with "interpolator."
	std::string basisName = basisEvaluatorName.substr(13, std::string::npos);
	if (verbose)
		display_message(INFORMATION_MESSAGE, "Using basis %s\n", basisName.c_str());

	std::string basisParametersTypeName = "parameters." + basisName;
	fmlBasisParametersType = this->libraryImport(basisParametersTypeName.c_str());
	if (fmlBasisParametersType == FML_INVALID_OBJECT_HANDLE)
		return FML_INVALID_OBJECT_HANDLE;

	std::string basisParametersArgumentName = basisParametersTypeName + ".argument";
	fmlBasisParametersArgument = this->libraryImport(basisParametersArgumentName.c_str());
	if (fmlBasisParametersArgument == FML_INVALID_OBJECT_HANDLE)
		return FML_INVALID_OBJECT_HANDLE;
	this->typeArgument[fmlBasisParametersType] = fmlBasisParametersArgument;

	std::string basisParametersComponentTypeName = basisParametersTypeName + ".component";
	fmlBasisParametersComponentType = this->libraryImport(basisParametersComponentTypeName.c_str());
	if (fmlBasisParametersComponentType == FML_INVALID_OBJECT_HANDLE)
		return FML_INVALID_OBJECT_HANDLE;

	std::string basisParametersComponentArgumentName = basisParametersComponentTypeName + ".argument";
	fmlBasisParametersComponentArgument = this->libraryImport(basisParametersComponentArgumentName.c_str());
	if (fmlBasisParametersComponentArgument == FML_INVALID_OBJECT_HANDLE)
		return FML_INVALID_OBJECT_HANDLE;
	this->typeArgument[fmlBasisParametersComponentType] = fmlBasisParametersComponentArgument;

	return fmlBasisEvaluator;
}

int FieldMLWriter::defineEnsembleFromLabels(FmlObjectHandle fmlEnsembleType, const DsLabels& labels)
{
	if (fmlEnsembleType == FML_INVALID_OBJECT_HANDLE)
		return CMZN_ERROR_GENERAL;
	int return_code = CMZN_OK;
	FmlErrorNumber fmlError;
	DsLabelIdentifier firstIdentifier, lastIdentifier; // used if contiguous
	DsLabelIdentifierRanges ranges;
	bool contiguous = labels.isContiguous();
	if (contiguous)
	{
		firstIdentifier = labels.getIdentifier(0);
		lastIdentifier = firstIdentifier + labels.getSize() - 1;
	}
	else
	{
		labels.getIdentifierRanges(ranges);
		if (ranges.size() == 1) // single range = contiguous
		{
			contiguous = true;
			firstIdentifier = ranges[0].first;
			lastIdentifier = ranges[0].last;
		}
	}
	if (contiguous)
	{
		fmlError = Fieldml_SetEnsembleMembersRange(this->fmlSession, fmlEnsembleType,
			firstIdentifier, lastIdentifier, /*stride*/1);
		if (fmlError != FML_OK)
			return_code = CMZN_ERROR_GENERAL;
	}
	else
	{
		// for non-contiguous use inline range data source
		std::string dataResourceName(labels.getName());
		dataResourceName += ".data.resource";
		FmlObjectHandle fmlDataResource = Fieldml_CreateInlineDataResource(this->fmlSession, dataResourceName.c_str());
		std::string dataSourceName(labels.getName());
		dataSourceName += ".data.source";
		FmlObjectHandle fmlDataSource = Fieldml_CreateArrayDataSource(this->fmlSession, dataSourceName.c_str(), fmlDataResource, /*location*/"0", /*rank*/2);
		int sizes[2] = { static_cast<int>(ranges.size()), 2 };
		Fieldml_SetArrayDataSourceRawSizes(this->fmlSession, fmlDataSource, sizes);
		Fieldml_SetArrayDataSourceSizes(this->fmlSession, fmlDataSource, sizes);
		FmlWriterHandle fmlArrayWriter = Fieldml_OpenArrayWriter(this->fmlSession,
			/*objectHandle*/fmlDataSource,
			/*typeHandle*/fmlEnsembleType,
			/*append*/false,
			sizes, // GRC OUTVALUE?
			/*rank*/2
			);
		if (fmlArrayWriter == FML_INVALID_OBJECT_HANDLE)
			return_code = CMZN_ERROR_GENERAL;
		if (CMZN_OK == return_code)
		{
			int numberOfRanges = static_cast<int>(ranges.size());
			for (int i = 0; i < numberOfRanges; ++i)
			{
				const int slabOffsets[] = { i, 0 };
				const int slabSizes[] = { 1, 2 };
				const int range[2] = { ranges[i].first, ranges[i].last };
				FmlIoErrorNumber fmlIoError = Fieldml_WriteIntSlab(fmlArrayWriter, slabOffsets, slabSizes, range);
				if (FML_IOERR_NO_ERROR != fmlIoError)
				{
					return_code = CMZN_ERROR_GENERAL;
					break;
				}
			}
		}
		Fieldml_CloseWriter(fmlArrayWriter);
		if (CMZN_OK == return_code)
		{
			fmlError = Fieldml_SetEnsembleMembersDataSource(this->fmlSession, fmlEnsembleType,
				FML_ENSEMBLE_MEMBER_RANGE_DATA, labels.getSize(), fmlDataSource);
			if (fmlError != FML_OK)
				return_code = CMZN_ERROR_GENERAL;
		}
	}
	return return_code;
}

// template and full specialisations to write different types with template
template <typename VALUETYPE> FmlIoErrorNumber FieldML_WriteSlab(
	FmlWriterHandle writerHandle, const int *offsets, const int *sizes, const VALUETYPE *valueBuffer);

template <> inline FmlIoErrorNumber FieldML_WriteSlab(
	FmlWriterHandle writerHandle, const int *offsets, const int *sizes, const double *valueBuffer)
{
	return Fieldml_WriteDoubleSlab(writerHandle, offsets, sizes, valueBuffer);
}

template <> inline FmlIoErrorNumber FieldML_WriteSlab(
	FmlWriterHandle writerHandle, const int *offsets, const int *sizes, const int *valueBuffer)
{
	return Fieldml_WriteIntSlab(writerHandle, offsets, sizes, valueBuffer);
}

template <typename VALUETYPE> const char *FieldML_valueFormat(const VALUETYPE *);

template <>	inline const char *FieldML_valueFormat(const double *)
{
	return " %.17g";
}

template <>	inline const char *FieldML_valueFormat(const int *)
{
	return " %d";
}

/** Write parameters in dense format, where parameters exist for all
  * permutations of all indexes. Currently only writes to inline text format.
  * @param name  The name of the parameter evaluator to return. Other
  * FieldML objects use this as a base name and append extra text.
  * @param denseIndexCount  Size of fmlDenseIndexArguments, equals rank of array.
  * @param fmlDenseIndexArguments  Array of dense argument evaluator handles.
  * Must be predefined ensemble value type.
  * @param parameterGenerator  Object implementing dense parameter generator interface.
  * @return  Handle to parameters object. */
template <typename VALUETYPE, class PARAMETERGENERATOR>
FmlObjectHandle FieldMLWriter::writeDenseParameters(const std::string& name,
	FmlObjectHandle fmlValueType, PARAMETERGENERATOR& parameterGenerator) const
{
	int denseIndexCount;
	const FmlObjectHandle *fmlDenseIndexArguments = parameterGenerator.getDenseArguments(denseIndexCount);

	std::string dataResourceName(name + ".data.resource");
	FmlObjectHandle fmlDataResource = Fieldml_CreateInlineDataResource(this->fmlSession, dataResourceName.c_str());
	std::string dataSourceName(name + ".data.source");
	FmlObjectHandle fmlDataSource = Fieldml_CreateArrayDataSource(this->fmlSession, dataSourceName.c_str(),
		fmlDataResource, /*location*/"1", /*rank*/denseIndexCount);
	std::vector<int> sizes(denseIndexCount);
	std::vector<int> offsets(denseIndexCount);
	for (int d = 0; d < denseIndexCount; ++d)
	{
		FmlObjectHandle fmlEnsembleType = Fieldml_GetValueType(this->fmlSession, fmlDenseIndexArguments[d]);
		sizes[d] = Fieldml_GetMemberCount(this->fmlSession, fmlEnsembleType);
		offsets[d] = 0.0;
	}
	Fieldml_SetArrayDataSourceRawSizes(this->fmlSession, fmlDataSource, sizes.data());
	Fieldml_SetArrayDataSourceSizes(this->fmlSession, fmlDataSource, sizes.data());
	FmlWriterHandle fmlArrayWriter = Fieldml_OpenArrayWriter(this->fmlSession,
		fmlDataSource, fmlValueType, /*append*/false, sizes.data(), /*rank*/denseIndexCount);
	if (fmlArrayWriter == FML_INVALID_OBJECT_HANDLE)
		return FML_INVALID_OBJECT_HANDLE;

	bool failed = false;
	const int *recordSizes = parameterGenerator.getDenseRecordSizes();
	for (int r = parameterGenerator.getRecordCount(); 0 < r; --r)
	{
		if (!parameterGenerator.nextRecord())
		{
			display_message(ERROR_MESSAGE, "FieldML Writer:  Too few parameters for evaluator %s", name.c_str());
			failed = true;
			break;
		}
		FmlIoErrorNumber fmlIoError = FieldML_WriteSlab(fmlArrayWriter,
			parameterGenerator.getRecordOffsets(), recordSizes, parameterGenerator.getRecordValues());
		if (FML_IOERR_NO_ERROR != fmlIoError)
		{
			failed = true;
			break;
		}
	}
	Fieldml_CloseWriter(fmlArrayWriter);
	if (failed)
		return FML_INVALID_OBJECT_HANDLE;

	FmlErrorNumber fmlError;
	FmlObjectHandle fmlParameters = FML_INVALID_OBJECT_HANDLE;
	fmlParameters = Fieldml_CreateParameterEvaluator(this->fmlSession, name.c_str(), fmlValueType);
	fmlError = Fieldml_SetParameterDataDescription(this->fmlSession, fmlParameters, FML_DATA_DESCRIPTION_DENSE_ARRAY);
	if (FML_OK != fmlError)
		return FML_INVALID_OBJECT_HANDLE;
	fmlError = Fieldml_SetDataSource(this->fmlSession, fmlParameters, fmlDataSource);
	if (FML_OK != fmlError)
		return FML_INVALID_OBJECT_HANDLE;
	for (int d = 0; d < denseIndexCount; ++d)
	{
		fmlError = Fieldml_AddDenseIndexEvaluator(this->fmlSession, fmlParameters,
			fmlDenseIndexArguments[d], /*orderHandle*/FML_INVALID_OBJECT_HANDLE);
		if (FML_OK != fmlError)
			return FML_INVALID_OBJECT_HANDLE;
	}
	return fmlParameters;
}

/** Write parameters in sparse format, where sparse indexes are written
  * 1:1 with the dense parameters they label, and which have a parameter for
  * all permutations of the dense indexes. Currently writes to inline text
  * format with indexes followed by dense parameters.
  * @param name  The name of the parameter evaluator to return. Other
  * FieldML objects use this as a base name and append extra text.
  * @param sparseIndexCount  Size of fmlDenseIndexArguments
  * @param fmlSparseIndexArguments  Array of sparse argument evaluator handles.
  * Must be predefined ensemble value type.
  * @param denseIndexCount  Size of fmlDenseIndexArguments
  * @param fmlDenseIndexArguments  Array of dense argument evaluator handles.
  * Must be predefined ensemble value type.
  * @return  Handle to parameters object.
  */
template <typename VALUETYPE, class PARAMETERGENERATOR>
FmlObjectHandle FieldMLWriter::writeSparseParameters(const std::string& name,
	FmlObjectHandle fmlValueType, PARAMETERGENERATOR& parameterGenerator) const
{
	int sparseIndexCount;
	const FmlObjectHandle *fmlSparseIndexArguments = parameterGenerator.getSparseArguments(sparseIndexCount);
	int denseIndexCount;
	const FmlObjectHandle *fmlDenseIndexArguments = parameterGenerator.getDenseArguments(denseIndexCount);
	std::string dataResourceName(name + ".data.resource");
	FmlObjectHandle fmlDataResource = Fieldml_CreateInlineDataResource(this->fmlSession, dataResourceName.c_str());
	// when writing to a text bulk data format we want the sparse labels to
	// precede the dense data under those labels (so kept together). This can only
	// be done if both are rank 2. Must confirm than the FieldML API can accept a
	// rank 2 data source for sparse data with more than 1 dense indexes.
	// This requires the second size to match product of dense index sizes.
	// Later: With HDF5 we need separate integer key and real data arrays.
	std::string keyDataSourceName(name + ".key.data.source");
	FmlObjectHandle fmlKeyDataSource = Fieldml_CreateArrayDataSource(this->fmlSession, keyDataSourceName.c_str(),
		fmlDataResource, /*location*/"1", /*rank*/2);
	std::string dataSourceName(name + ".data.source");
	FmlObjectHandle fmlDataSource = Fieldml_CreateArrayDataSource(this->fmlSession, dataSourceName.c_str(),
		fmlDataResource, /*location*/"1", /*rank*/2);
	if ((fmlKeyDataSource == FML_INVALID_OBJECT_HANDLE) || (fmlDataSource == FML_INVALID_OBJECT_HANDLE))
		return FML_INVALID_OBJECT_HANDLE;

	const int *recordSizes = parameterGenerator.getDenseRecordSizes();
	int denseSize = 1;
	for (int i = 0; i < denseIndexCount; ++i)
		denseSize *= recordSizes[i];

	const int recordCount = parameterGenerator.getRecordCount();

	const int rawSizes[2] = { recordCount, sparseIndexCount + denseSize };
	const int keySizes[2] = { recordCount, sparseIndexCount };
	const int keyOffsets[2] = { 0, 0 };
	const int sizes[2] = { recordCount, denseSize };
	const int offsets[2] = { 0, sparseIndexCount };
	Fieldml_SetArrayDataSourceRawSizes(this->fmlSession, fmlKeyDataSource, const_cast<int*>(rawSizes));
	Fieldml_SetArrayDataSourceSizes(this->fmlSession, fmlKeyDataSource, const_cast<int*>(keySizes));
	Fieldml_SetArrayDataSourceOffsets(this->fmlSession, fmlKeyDataSource, const_cast<int*>(keyOffsets));
	Fieldml_SetArrayDataSourceRawSizes(this->fmlSession, fmlDataSource, const_cast<int*>(rawSizes));
	Fieldml_SetArrayDataSourceSizes(this->fmlSession, fmlDataSource, const_cast<int*>(sizes));
	Fieldml_SetArrayDataSourceOffsets(this->fmlSession, fmlDataSource, const_cast<int*>(offsets));

	std::ostringstream stringStream;
	stringStream << "\n";
	// Future: configurable numerical format for reals
	const VALUETYPE *values = 0;
	const char *valueFormat = FieldML_valueFormat(values);
	char tmpValueString[50];
	const int *indexes;
	bool failed = false;
	for (int r = 0; r < recordCount; ++r)
	{
		if (!parameterGenerator.nextRecord())
		{
			display_message(ERROR_MESSAGE, "FieldML Writer:  Too few parameters for evaluator %s", name.c_str());
			failed = true;
			break;
		}
		indexes = parameterGenerator.getRecordIndexes();
		for (int s = 0; s < sparseIndexCount; ++s)
			stringStream << " " << indexes[s];
		values = parameterGenerator.getRecordValues();
		for (int d = 0; d < denseSize; ++d)
		{
			sprintf(tmpValueString, valueFormat, values[d]);
			stringStream << tmpValueString;
		}
		stringStream << "\n";
	}
	if (failed)
		return FML_INVALID_OBJECT_HANDLE;
	// following call copies all the data so expensive; best solution is to not use inline data,
	// but could implement own memory stream, or do so within the FieldML API
	std::string sstring = stringStream.str();
	int sstringSize = static_cast<int>(sstring.size());
	FmlErrorNumber fmlError;
	if (FML_OK != (fmlError = Fieldml_SetInlineData(this->fmlSession, fmlDataResource, sstring.c_str(), sstringSize)))
	{
		display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to set inline data for parameters %s", name.c_str());
		return FML_INVALID_OBJECT_HANDLE;
	}
	FmlObjectHandle fmlParameters = FML_INVALID_OBJECT_HANDLE;
	fmlParameters = Fieldml_CreateParameterEvaluator(this->fmlSession, name.c_str(), fmlValueType);
	if ((FML_OK != (fmlError = Fieldml_SetParameterDataDescription(this->fmlSession, fmlParameters, FML_DATA_DESCRIPTION_DOK_ARRAY)))
		|| (FML_OK != (fmlError = Fieldml_SetKeyDataSource(this->fmlSession, fmlParameters, fmlKeyDataSource)))
		|| (FML_OK != (fmlError = Fieldml_SetDataSource(this->fmlSession, fmlParameters, fmlDataSource))))
	{
		display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to set attributes for parameters %s", name.c_str());
		return FML_INVALID_OBJECT_HANDLE;
	}
	for (int s = 0; s < sparseIndexCount; ++s)
	{
		if (FML_OK != (fmlError = Fieldml_AddSparseIndexEvaluator(this->fmlSession, fmlParameters, fmlSparseIndexArguments[s])))
		{
			display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to set sparse index for parameters %s", name.c_str());
			return FML_INVALID_OBJECT_HANDLE;
		}
	}
	for (int d = 0; d < denseIndexCount; ++d)
	{
		if (FML_OK != (fmlError = Fieldml_AddDenseIndexEvaluator(this->fmlSession, fmlParameters,
			fmlDenseIndexArguments[d], /*orderHandle*/FML_INVALID_OBJECT_HANDLE)))
		{
			display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to set dense index for parameters %s", name.c_str());
			return FML_INVALID_OBJECT_HANDLE;
		}
	}
	return fmlParameters;
}

template <typename VALUETYPE> FmlObjectHandle FieldMLWriter::defineParametersFromMap(
	DsMap<VALUETYPE>& parameterMap, FmlObjectHandle fmlValueType)
{
	std::string name = parameterMap.getName();
	std::vector<HCDsLabels> sparseLabelsArray;
	std::vector<HCDsLabels> denseLabelsArray;
	parameterMap.getSparsity(sparseLabelsArray, denseLabelsArray);
	std::string dataResourceName(name + ".data.resource");
	FmlObjectHandle fmlDataResource = Fieldml_CreateInlineDataResource(this->fmlSession, dataResourceName.c_str());
	const int denseLabelsCount = static_cast<int>(denseLabelsArray.size());
	const int sparseLabelsCount = static_cast<int>(sparseLabelsArray.size());
	std::string dataSourceName(name + ".data.source");
	int return_code = CMZN_OK;
	FmlErrorNumber fmlError;
	FmlObjectHandle fmlDataSource = FML_INVALID_OBJECT_HANDLE;
	FmlObjectHandle fmlKeyDataSource = FML_INVALID_OBJECT_HANDLE;
	if (sparseLabelsCount > 0)
	{
		// when writing to a text bulk data format we want the sparse labels to
		// precede the dense data under those labels (so kept together). This can only
		// be done if both are rank 2. Must confirm than the FieldML API can accept a
		// rank 2 data source for sparse data with more than 1 dense indexes.
		// This requires the second size to match product of dense index sizes.
		// Later: With HDF5 we need separate integer key and real data arrays.
		fmlDataSource = Fieldml_CreateArrayDataSource(this->fmlSession, dataSourceName.c_str(),
			fmlDataResource, /*location*/"1", /*rank*/2);
		std::string indexDataSourceName(name + ".key.data.source");
		fmlKeyDataSource = Fieldml_CreateArrayDataSource(this->fmlSession, indexDataSourceName.c_str(),
			fmlDataResource, /*location*/"1", /*rank*/2);
		int denseSize = 1;
		for (int i = 0; i < denseLabelsCount; ++i)
			denseSize *= denseLabelsArray[i]->getSize();
		// start with 1 record and revise once known
		int numberOfRecords = 1;
		int rawSizes[2] = { numberOfRecords, sparseLabelsCount + denseSize };
		int sizes[2] = { numberOfRecords, denseSize };
		int offsets[2] = { 0, sparseLabelsCount };
		int keySizes[2] = { numberOfRecords, sparseLabelsCount };
		int keyOffsets[2]= { 0, 0 };
		Fieldml_SetArrayDataSourceRawSizes(this->fmlSession, fmlDataSource, rawSizes);
		Fieldml_SetArrayDataSourceSizes(this->fmlSession, fmlDataSource, sizes);
		Fieldml_SetArrayDataSourceOffsets(this->fmlSession, fmlDataSource, offsets);
		Fieldml_SetArrayDataSourceRawSizes(this->fmlSession, fmlKeyDataSource, rawSizes);
		Fieldml_SetArrayDataSourceSizes(this->fmlSession, fmlKeyDataSource, keySizes);
		Fieldml_SetArrayDataSourceOffsets(this->fmlSession, fmlKeyDataSource, keyOffsets);
		numberOfRecords = 0;
		HDsMapIndexing mapIndexing(parameterMap.createIndexing());
		for (int i = 0; i < sparseLabelsCount; ++i)
			mapIndexing->setEntryIndex(*sparseLabelsArray[i], DS_LABEL_INDEX_INVALID);
		mapIndexing->resetSparseIterators();
		VALUETYPE *denseValues = new VALUETYPE[denseSize];
		if (denseSize && (!denseValues))
			return_code = CMZN_ERROR_MEMORY;
		else
		{
			std::ostringstream stringStream;
			stringStream << "\n";
			// Future: configurable numerical format for reals
			const char *valueFormat = FieldML_valueFormat(denseValues);
			char tmpValueString[50];
			while (parameterMap.incrementSparseIterators(*mapIndexing))
			{
				if (parameterMap.getValues(*mapIndexing, denseSize, denseValues))
				{
					++numberOfRecords;
					for (int i = 0; i < sparseLabelsCount; ++i)
					{
						DsLabelIdentifier identifier = mapIndexing->getSparseIdentifier(i);
						if (i > 0)
							stringStream << " ";
						stringStream << identifier;
					}
					for (int i = 0; i < denseSize; ++i)
					{
						sprintf(tmpValueString, valueFormat, denseValues[i]);
						stringStream << tmpValueString;
					}
					stringStream << "\n";
				}
				else
				{
					display_message(ERROR_MESSAGE, "FieldML Writer:  "
						"Failed to get sparsely indexed values from map %s", parameterMap.getName().c_str());
					return_code = CMZN_ERROR_GENERAL;
					break;
				}
			}
			std::string sstring = stringStream.str();
			int sstringSize = static_cast<int>(sstring.size());
			fmlError = Fieldml_SetInlineData(this->fmlSession, fmlDataResource, sstring.c_str(), sstringSize);
			if (FML_OK != fmlError)
				return_code = CMZN_ERROR_GENERAL;
		}
		delete[] denseValues;
		rawSizes[0] = numberOfRecords;
		sizes[0] = numberOfRecords;
		keySizes[0] = numberOfRecords;
		Fieldml_SetArrayDataSourceRawSizes(this->fmlSession, fmlDataSource, rawSizes);
		Fieldml_SetArrayDataSourceSizes(this->fmlSession, fmlDataSource, sizes);
		Fieldml_SetArrayDataSourceRawSizes(this->fmlSession, fmlKeyDataSource, rawSizes);
		Fieldml_SetArrayDataSourceSizes(this->fmlSession, fmlKeyDataSource, keySizes);
	}
	else
	{
		fmlDataSource = Fieldml_CreateArrayDataSource(this->fmlSession, dataSourceName.c_str(),
			fmlDataResource, /*location*/"0", /*rank*/denseLabelsCount);
		int *sizes = new int[denseLabelsCount];
		int *offsets = new int[denseLabelsCount];
		for (int i = 0; i < denseLabelsCount; ++i)
		{
			sizes[i] = denseLabelsArray[i]->getSize();
			offsets[i] = 0;
		}
		Fieldml_SetArrayDataSourceRawSizes(this->fmlSession, fmlDataSource, sizes);
		Fieldml_SetArrayDataSourceSizes(this->fmlSession, fmlDataSource, sizes);

		FmlWriterHandle fmlArrayWriter = Fieldml_OpenArrayWriter(this->fmlSession,
			fmlDataSource, fmlValueType, /*append*/false,
			sizes, // GRC OUTVALUE?
			/*rank*/denseLabelsCount
			);

		HDsMapIndexing mapIndexing(parameterMap.createIndexing());
		DsMapAddressType denseValuesCount = mapIndexing->getEntryCount();
		VALUETYPE *values = new VALUETYPE[denseValuesCount];
		if (fmlArrayWriter == FML_INVALID_OBJECT_HANDLE)
			return_code = CMZN_ERROR_GENERAL;
		if (CMZN_OK == return_code)
		{
			if (!parameterMap.getValues(*mapIndexing, denseValuesCount, values))
				return_code = CMZN_ERROR_GENERAL;
			else
			{
				FmlIoErrorNumber fmlIoError = FieldML_WriteSlab(fmlArrayWriter, offsets, sizes, values);
				if (FML_IOERR_NO_ERROR != fmlIoError)
					return_code = CMZN_ERROR_GENERAL;
			}
		}
		Fieldml_CloseWriter(fmlArrayWriter);
		delete[] values;
		delete[] offsets;
		delete[] sizes;
	}
	FmlObjectHandle fmlParameters = FML_INVALID_OBJECT_HANDLE;
	if (CMZN_OK == return_code)
	{
		fmlParameters = Fieldml_CreateParameterEvaluator(this->fmlSession, name.c_str(), fmlValueType);
		fmlError = Fieldml_SetParameterDataDescription(this->fmlSession, fmlParameters,
			(0 == sparseLabelsCount) ? FML_DATA_DESCRIPTION_DENSE_ARRAY : FML_DATA_DESCRIPTION_DOK_ARRAY);
		if (FML_OK != fmlError)
			return_code = CMZN_ERROR_GENERAL;
		fmlError = Fieldml_SetDataSource(this->fmlSession, fmlParameters, fmlDataSource);
		if (FML_OK != fmlError)
			return_code = CMZN_ERROR_GENERAL;
		if (0 < sparseLabelsCount)
		{
			fmlError = Fieldml_SetKeyDataSource(this->fmlSession, fmlParameters, fmlKeyDataSource);
			if (FML_OK != fmlError)
				return_code = CMZN_ERROR_GENERAL;
			for (int i = 0; i < sparseLabelsCount; ++i)
			{
				std::string labelsName = sparseLabelsArray[i]->getName();
				FmlObjectHandle fmlLabelsType = Fieldml_GetObjectByName(this->fmlSession, labelsName.c_str());
				FmlObjectHandle fmlIndexArgument = this->getArgumentForType(fmlLabelsType);
				fmlError = Fieldml_AddSparseIndexEvaluator(this->fmlSession, fmlParameters,
					fmlIndexArgument);
				if (FML_OK != fmlError)
				{
					return_code = CMZN_ERROR_GENERAL;
					break;
				}
			}
		}
		for (int i = 0; i < denseLabelsCount; ++i)
		{
			std::string labelsName = denseLabelsArray[i]->getName();
			FmlObjectHandle fmlLabelsType = Fieldml_GetObjectByName(this->fmlSession, labelsName.c_str());
			FmlObjectHandle fmlIndexArgument = this->getArgumentForType(fmlLabelsType);
			fmlError = Fieldml_AddDenseIndexEvaluator(this->fmlSession, fmlParameters,
				fmlIndexArgument, /*orderHandle*/FML_INVALID_OBJECT_HANDLE);
			if (FML_OK != fmlError)
			{
				return_code = CMZN_ERROR_GENERAL;
				break;
			}
		}
	}
	if (CMZN_OK != return_code)
		return FML_INVALID_OBJECT_HANDLE;
	return fmlParameters;
}

int FieldMLWriter::writeMesh(int meshDimension, bool writeIfEmpty)
{
	FE_mesh *mesh = FE_region_find_FE_mesh_by_dimension(this->fe_region, meshDimension);
	const DsLabels& elementLabels = mesh->getLabels();
	if ((elementLabels.getSize() == 0) && (!writeIfEmpty))
		return CMZN_OK;
	FmlErrorNumber fmlError;
	FmlObjectHandle fmlMeshType = Fieldml_CreateMeshType(this->fmlSession, mesh->getName());
	const char *meshChartName = "xi";
	FmlObjectHandle fmlMeshChartType = Fieldml_CreateMeshChartType(this->fmlSession, fmlMeshType, meshChartName);
	FmlObjectHandle fmlMeshChartComponentsType = FML_INVALID_OBJECT_HANDLE;
	if (fmlMeshChartType == FML_INVALID_OBJECT_HANDLE)
		return CMZN_ERROR_GENERAL;
	else
	{
		// since chart.1d in the FieldML library has a component ensemble with 1 member,
		// we are required to do the same for meshes to bind with it.
		// Hence following is not conditional on: if (meshDimension > 1)
		const char *chartComponentsName = "mesh3d.xi.components";
		fmlMeshChartComponentsType = Fieldml_CreateContinuousTypeComponents(
			this->fmlSession, fmlMeshChartType, chartComponentsName, meshDimension);
		fmlError = Fieldml_SetEnsembleMembersRange(this->fmlSession, fmlMeshChartComponentsType,
			1, meshDimension, /*stride*/1);
		if (fmlMeshChartComponentsType == FML_INVALID_OBJECT_HANDLE)
			return CMZN_ERROR_GENERAL;
	}
	const char *elementsName = "elements";
	FmlObjectHandle fmlElementsType = Fieldml_CreateMeshElementsType(this->fmlSession, fmlMeshType, elementsName);
	int result = this->defineEnsembleFromLabels(fmlElementsType, elementLabels);
	if (result != CMZN_OK)
		return result;
	this->fmlMeshElementsType[meshDimension] = fmlElementsType;

	// ensure we have argument for mesh type and can find argument for elements and xi type
	// since it uses a special naming pattern e.g. mesh3d.argument.elements|xi
	this->getArgumentForType(fmlMeshType);

	std::string elementsArgumentName(mesh->getName());
	elementsArgumentName += ".argument.";
	elementsArgumentName += elementsName;
	FmlObjectHandle fmlElementsArgument = Fieldml_GetObjectByName(this->fmlSession, elementsArgumentName.c_str());
	if (fmlElementsArgument == FML_INVALID_OBJECT_HANDLE)
		return CMZN_ERROR_GENERAL;
	this->typeArgument[fmlElementsType] = fmlElementsArgument;

	std::string meshChartArgumentName(mesh->getName());
	meshChartArgumentName += ".argument.";
	meshChartArgumentName += meshChartName;
	FmlObjectHandle fmlMeshChartArgument = Fieldml_GetObjectByName(this->fmlSession, meshChartArgumentName.c_str());
	if (fmlMeshChartArgument == FML_INVALID_OBJECT_HANDLE)
		return CMZN_ERROR_GENERAL;
	this->typeArgument[fmlMeshChartType] = fmlMeshChartArgument;

	// set up shape evaluator, single fixed or indirectly mapped
	const int shapeCount = mesh->getElementShapeFacesCount();
	// GRC: doesn't handle 0 elements, 0 shapes
	FmlObjectHandle fmlMeshShapeEvaluator;
	if (1 == shapeCount)
	{
		// direct use of single library shape evaluator
		cmzn_element_shape_type elementShapeType = mesh->getElementShapeTypeAtIndex(0);
		const char *elementShapeName = getFieldmlNameFromElementShape(elementShapeType);
		fmlMeshShapeEvaluator = this->libraryImport(elementShapeName);
	}
	else
	{
		// indirect map to multiple shapes evaluators
		// 1. define shape ID ensemble type for all shapes in use
		std::string meshShapeIdsName(mesh->getName());
		meshShapeIdsName += ".shapeids";
		FmlObjectHandle fmlMeshShapeIdsType = Fieldml_CreateEnsembleType(this->fmlSession, meshShapeIdsName.c_str());
		fmlError = Fieldml_SetEnsembleMembersRange(this->fmlSession, fmlMeshShapeIdsType, 1, shapeCount, /*stride*/1);
		if (FML_OK != fmlError)
		{
			display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to create mesh shape ID type");
			return CMZN_ERROR_GENERAL;
		}
		// 2. define a parameter evaluator giving map from element to shape ID
		std::string meshShapeIdsMapName(mesh->getName());
		meshShapeIdsMapName += ".shapemap";
		FE_mesh_element_shape_type_parameter_generator shapeIdsParameterGenerator(mesh, fmlElementsArgument);
		FmlObjectHandle fmlMeshShapeIdsMap = this->writeDenseParameters<int>(meshShapeIdsMapName, fmlMeshShapeIdsType, shapeIdsParameterGenerator);
		if (fmlMeshShapeIdsMap == FML_INVALID_OBJECT_HANDLE)
		{
			display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to create mesh element to shape ID map");
			return CMZN_ERROR_GENERAL;
		}
		// 3. define piecewise evaluator mapping shape ID to library shape evaluator
		std::string meshShapeEvaluatorName(mesh->getName());
		meshShapeEvaluatorName += ".shape";
		FmlObjectHandle fmlBooleanType = this->libraryImport("boolean");
		fmlMeshShapeEvaluator = Fieldml_CreatePiecewiseEvaluator(this->fmlSession,
			meshShapeEvaluatorName.c_str(), fmlBooleanType);
		FmlObjectHandle fmlMeshShapeIdsArgument = this->getArgumentForType(fmlMeshShapeIdsType);
		fmlError = Fieldml_SetIndexEvaluator(this->fmlSession, fmlMeshShapeEvaluator, /*index*/1, fmlMeshShapeIdsArgument);
		if (FML_OK != fmlError)
			return CMZN_ERROR_GENERAL;
		for (int i = 0; i < shapeCount; ++i)
		{
			cmzn_element_shape_type elementShapeType = mesh->getElementShapeTypeAtIndex(i);
			const char *elementShapeName = getFieldmlNameFromElementShape(elementShapeType);
			FmlObjectHandle fmlElementShapeEvaluator = this->libraryImport(elementShapeName);
			fmlError = Fieldml_SetEvaluator(this->fmlSession, fmlMeshShapeEvaluator, i + 1, fmlElementShapeEvaluator);
			if (FML_OK != fmlError)
				return CMZN_ERROR_GENERAL;
		}
		fmlError = Fieldml_SetBind(this->fmlSession, fmlMeshShapeEvaluator, fmlMeshShapeIdsArgument, fmlMeshShapeIdsMap);
		if (FML_OK != fmlError)
			return CMZN_ERROR_GENERAL;
	}
	fmlError = Fieldml_SetMeshShapes(this->fmlSession, fmlMeshType, fmlMeshShapeEvaluator);
	if (fmlError != FML_OK)
	{
		display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to set mesh element shape evaluator");
		return CMZN_ERROR_GENERAL;
	}
	return CMZN_OK;
}

int FieldMLWriter::getHighestMeshDimension() const
{
	return FE_region_get_highest_dimension(this->region->get_FE_region());
}

// Ensures the versions ensemble and labels have at least as many entries as the
// specified minimum.
int FieldMLWriter::setMinimumNodeVersions(int minimumNodeVersions)
{
	int currentMaximumNodeVersions = this->nodeVersions->getSize();
	if (minimumNodeVersions > currentMaximumNodeVersions)
	{
		int result = this->nodeVersions->addLabelsRange(currentMaximumNodeVersions + 1, minimumNodeVersions);
		if (result != CMZN_OK)
			return result;
		FmlErrorNumber fmlError = Fieldml_SetEnsembleMembersRange(this->fmlSession,
			this->fmlNodeVersionsType, 1, minimumNodeVersions, /*stride*/1);
		if (fmlError != FML_OK)
			return CMZN_ERROR_GENERAL;
		// need a constant evaluator to address each version
		char idString[30];
		char nodeVersionConstantName[30];
		for (int v = currentMaximumNodeVersions; v < minimumNodeVersions; ++v)
		{
			sprintf(idString, "%d", v + 1);
			sprintf(nodeVersionConstantName, "node_versions.%d", v + 1);
			FmlObjectHandle fmlNodeVersionConstant = Fieldml_CreateConstantEvaluator(this->fmlSession, nodeVersionConstantName, idString, this->fmlNodeVersionsType);
			if (fmlNodeVersionConstant == FML_INVALID_OBJECT_HANDLE)
			{
				display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to create node version constants");
				return CMZN_ERROR_GENERAL;
			}
			this->fmlNodeVersionConstants.push_back(fmlNodeVersionConstant);
		}
	}
	return CMZN_OK;
}

int FieldMLWriter::writeNodeset(cmzn_field_domain_type domainType, bool writeIfEmpty)
{
	const FE_nodeset *nodeset = FE_region_find_FE_nodeset_by_field_domain_type(this->fe_region, domainType);
	const DsLabels& nodeLabels = nodeset->getLabels();
	if ((nodeLabels.getSize() == 0) && (!writeIfEmpty))
		return CMZN_OK;
	FmlObjectHandle fmlNodesType = Fieldml_CreateEnsembleType(this->fmlSession, nodeset->getName());
	int return_code = this->defineEnsembleFromLabels(fmlNodesType, nodeLabels);
	if (CMZN_OK != return_code)
		return return_code;
	this->fmlNodesTypes[domainType] = fmlNodesType;
	if (!this->nodeDerivatives)
	{
		std::string nodeDerivativesTypeName("node_derivatives");
		cmzn::SetImpl(this->nodeDerivatives, new DsLabels());
		this->nodeDerivatives->setName(nodeDerivativesTypeName);
		this->nodeDerivatives->addLabelsRange(1, 8);
		this->fmlNodeDerivativesType = Fieldml_CreateEnsembleType(this->fmlSession, nodeDerivativesTypeName.c_str());
		return_code = this->defineEnsembleFromLabels(this->fmlNodeDerivativesType, *this->nodeDerivatives);
		if (CMZN_OK != return_code)
			return return_code;
		// need a constant evaluator to address each derivative use descriptive names: value d_ds1 etc.
		char idString[30];
		for (int d = 0; d < 8; ++d)
		{
			sprintf(idString, "%d", d + 1);
			std::string nodeDerivativeConstantName = nodeDerivativesTypeName + "." + derivativeNames[d];
			FmlObjectHandle fmlNodeDerivativeConstant = Fieldml_CreateConstantEvaluator(this->fmlSession, nodeDerivativeConstantName.c_str(), idString, this->fmlNodeDerivativesType);
			if (fmlNodeDerivativeConstant == FML_INVALID_OBJECT_HANDLE)
			{
				display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to create node derivative constants");
				return CMZN_ERROR_GENERAL;
			}
			this->fmlNodeDerivativeConstants.push_back(fmlNodeDerivativeConstant);
		}
		std::string nodeVersionsTypeName("node_versions");
		cmzn::SetImpl(this->nodeVersions, new DsLabels());
		this->nodeVersions->setName(nodeVersionsTypeName);
		this->fmlNodeVersionsType = Fieldml_CreateEnsembleType(this->fmlSession, nodeVersionsTypeName.c_str());
		if ((!this->nodeVersions) || (FML_INVALID_OBJECT_HANDLE == this->fmlNodeVersionsType))
			return CMZN_ERROR_GENERAL;
		return_code = this->setMinimumNodeVersions(1);
		if (CMZN_OK != return_code)
			return return_code;
	}
	std::string nodesetName(nodeset->getName());
	std::string nodesParametersArgumentName = nodesetName + ".parameters";
	FmlObjectHandle fmlRealType = this->libraryImport("real.1d");
	FmlObjectHandle fmlNodesArgument = this->getArgumentForType(fmlNodesType);
	this->fmlNodeDerivativesArgument = this->getArgumentForType(this->fmlNodeDerivativesType);
	this->fmlNodeVersionsArgument = this->getArgumentForType(this->fmlNodeVersionsType);
	FmlObjectHandle fmlNodesParametersArgument = Fieldml_CreateArgumentEvaluator(
		this->fmlSession, nodesParametersArgumentName.c_str(), fmlRealType);
	FmlErrorNumber fmlError = Fieldml_AddArgument(this->fmlSession, fmlNodesParametersArgument, fmlNodesArgument);
	if (FML_OK != fmlError)
		return_code = CMZN_ERROR_GENERAL;
	fmlError = Fieldml_AddArgument(this->fmlSession, fmlNodesParametersArgument, this->fmlNodeDerivativesArgument);
	if (FML_OK != fmlError)
		return_code = CMZN_ERROR_GENERAL;
	fmlError = Fieldml_AddArgument(this->fmlSession, fmlNodesParametersArgument, this->fmlNodeVersionsArgument);
	if (FML_OK != fmlError)
		return_code = CMZN_ERROR_GENERAL;
	this->fmlNodesParametersArguments[domainType] = fmlNodesParametersArgument;
	return CMZN_OK;
}

int FieldMLWriter::writeNodesets()
{
	int return_code = CMZN_OK;
	if (CMZN_OK == return_code)
		return_code = this->writeNodeset(CMZN_FIELD_DOMAIN_TYPE_NODES, /*writeIfEmpty*/true);
	//if (CMZN_OK == return_code)
	//	return_code = this->writeNodeset(CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS, /*writeIfEmpty*/false);
	return return_code;
}

FmlObjectHandle FieldMLWriter::getZeroEvaluator()
{
	if (this->fmlZeroEvaluator != FML_INVALID_OBJECT_HANDLE)
		return this->fmlZeroEvaluator;
	FmlObjectHandle fmlRealType = this->libraryImport("real.1d");
	this->fmlZeroEvaluator = Fieldml_CreateConstantEvaluator(this->fmlSession, "zero", "0", fmlRealType);
	if (this->fmlZeroEvaluator == FML_INVALID_OBJECT_HANDLE)
		display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to create zero evaluator");
	return this->fmlZeroEvaluator;
}

FmlObjectHandle FieldMLWriter::writeElementfieldtemplate(const FE_element_field_template *eft, const std::string& name,
	FmlObjectHandle& fmlEftNodesArgument, FmlObjectHandle& fmlEftNodeParametersArgument,
	FmlObjectHandle& fmlEftScaleFactorIndexesArgument, FmlObjectHandle& fmlEftScaleFactorsArgument)
{
	if (!(eft && eft->getMesh()))
		return FML_INVALID_OBJECT_HANDLE;

	FmlObjectHandle fmlBasisParametersType, fmlBasisParametersArgument,
		fmlBasisParametersComponentType, fmlBasisParametersComponentArgument;
	FmlObjectHandle fmlBasisEvaluator = this->getBasisEvaluator(eft->getBasis(),
		fmlBasisParametersType, fmlBasisParametersArgument,
		fmlBasisParametersComponentType, fmlBasisParametersComponentArgument);
	if (fmlBasisEvaluator == FML_INVALID_OBJECT_HANDLE)
	{
		display_message(INFORMATION_MESSAGE, "FieldML Writer:  Failed to get basis evaluator and associated data");
		return FML_INVALID_OBJECT_HANDLE;
	}

	if (eft->getParameterMappingMode() != CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE)
	{
		display_message(INFORMATION_MESSAGE, "FieldML Writer:  Only node-based element field templates are implemented");
		return FML_INVALID_OBJECT_HANDLE;
	}
	FmlErrorNumber fmlError;

	// set up eft local nodes and scale factors
	const int nodeCount = eft->getNumberOfLocalNodes();
	std::string eftNodesName = name + ".nodes";
	FmlObjectHandle fmlEftNodes = Fieldml_CreateEnsembleType(this->fmlSession, eftNodesName.c_str());
	fmlError = Fieldml_SetEnsembleMembersRange(this->fmlSession, fmlEftNodes, 1, nodeCount, /*stride*/1);
	if (FML_OK != fmlError)
	{
		display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to create EFT nodes");
		return FML_INVALID_OBJECT_HANDLE;
	}
	fmlEftNodesArgument = this->getArgumentForType(fmlEftNodes);

	// create EFT node parameters argument = real(local node, node derivatives, node versions)
	// to which the mesh element evaluator will later bind the global node parameters argument and
	// simultaneously bind the local to global node map
	FmlObjectHandle fmlRealType = this->libraryImport("real.1d");
	std::string eftNodeParametersArgumentName = name + ".nodeparameters.argument";
	fmlEftNodeParametersArgument = Fieldml_CreateArgumentEvaluator(this->fmlSession, eftNodeParametersArgumentName.c_str(), fmlRealType);
	if ((FML_OK != (fmlError = Fieldml_AddArgument(this->fmlSession, fmlEftNodeParametersArgument, fmlEftNodesArgument)))
		|| (FML_OK != (fmlError = Fieldml_AddArgument(this->fmlSession, fmlEftNodeParametersArgument, this->fmlNodeDerivativesArgument)))
		|| (FML_OK != (fmlError = Fieldml_AddArgument(this->fmlSession, fmlEftNodeParametersArgument, this->fmlNodeVersionsArgument))))
	{
		display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to create EFT node parameters argument");
		return FML_INVALID_OBJECT_HANDLE;
	}

	// need a constant evaluator to address each local node (eventually could be avoided by having a BindConstant feature)
	std::vector<FmlObjectHandle> fmlEftNodeIndexConstants(nodeCount, FML_INVALID_OBJECT_HANDLE);
	char idString[50];
	for (int n = 0; n < nodeCount; ++n)
	{
		sprintf(idString, "%d", n + 1);
		std::string eftNodeIndexConstantName = name + ".nodes." + idString;
		fmlEftNodeIndexConstants[n] = Fieldml_CreateConstantEvaluator(this->fmlSession, eftNodeIndexConstantName.c_str(), idString, fmlEftNodes);
		if (fmlEftNodeIndexConstants[n] == FML_INVALID_OBJECT_HANDLE)
		{
			display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to create EFT node index constants");
			return FML_INVALID_OBJECT_HANDLE;
		}
	}

	FmlObjectHandle fmlEftScaleFactorIndexes = FML_INVALID_OBJECT_HANDLE;
	const int scaleFactorCount = eft->getNumberOfLocalScaleFactors();
	if (scaleFactorCount > 0)
	{
		std::string eftScaleFactorIndexesName = name + ".scalefactorindexes";
		fmlEftScaleFactorIndexes = Fieldml_CreateEnsembleType(this->fmlSession, eftScaleFactorIndexesName.c_str());
		fmlError = Fieldml_SetEnsembleMembersRange(this->fmlSession, fmlEftScaleFactorIndexes, 1, scaleFactorCount, /*stride*/1);
		if (FML_OK != fmlError)
		{
			display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to create EFT scale factor indexes");
			return FML_INVALID_OBJECT_HANDLE;
		}
		fmlEftScaleFactorIndexesArgument = this->getArgumentForType(fmlEftScaleFactorIndexes);

		// need a constant evaluator to address each scale factor index (eventually could be avoided by having a BindConstant feature)
		std::vector<FmlObjectHandle> fmlEftScaleFactorIndexConstants(scaleFactorCount, FML_INVALID_OBJECT_HANDLE);
		for (int s = 0; s < scaleFactorCount; ++s)
		{
			sprintf(idString, "%d", s + 1);
			std::string eftScaleFactorIndexConstantName = name + ".scalefactorsindexes." + idString;
			fmlEftScaleFactorIndexConstants[s] = Fieldml_CreateConstantEvaluator(this->fmlSession, eftScaleFactorIndexConstantName.c_str(), idString, fmlEftScaleFactorIndexes);
			if (fmlEftScaleFactorIndexConstants[s] == FML_INVALID_OBJECT_HANDLE)
			{
				display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to create EFT scale factor index constants");
				return FML_INVALID_OBJECT_HANDLE;
			}
		}

		// argument: scale factors = real(scale factor indexes)
		std::string eftScaleFactorsArgumentName = name + ".scalefactors.argument";
		fmlEftScaleFactorsArgument = Fieldml_CreateArgumentEvaluator(this->fmlSession, eftScaleFactorsArgumentName.c_str(), fmlRealType);
		if (FML_OK != (fmlError = Fieldml_AddArgument(this->fmlSession, fmlEftScaleFactorsArgument, fmlEftScaleFactorIndexesArgument)))
		{
			display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to create EFT scale factors argument");
			return FML_INVALID_OBJECT_HANDLE;
		}
	}

	// get evaluators for each parameter for the eft's basis
	const int functionCount = eft->getNumberOfFunctions();
	std::vector<FmlObjectHandle> fmlParameterComponents(functionCount, FML_INVALID_OBJECT_HANDLE);
	for (int f = 0; f < functionCount; ++f)
	{
		const int termCount = eft->getFunctionNumberOfTerms(f);
		if (0 == termCount)
		{
			fmlParameterComponents[f] = this->getZeroEvaluator();
		}
		else
		{
			if (termCount > 1)
				display_message(INFORMATION_MESSAGE, "FieldML Writer:  General linear map not yet implemented; writing first term only"); // GRC TODO
			for (int t = 0; t < 1/*termCount*/; ++t) // GRC limited to 1 for now
			{
				const int scalingCount = eft->getTermScalingCount(f, t);
				if (scalingCount > 0)
					display_message(WARNING_MESSAGE, "FieldML Writer:  Scaling not yet implemented; omitting"); // GRC TODO
				const int localNodeIndex = eft->getTermLocalNodeIndex(f, t);
				const int derivativeIndex = eft->getTermNodeValueLabel(f, t) - CMZN_NODE_VALUE_LABEL_VALUE;
				if ((derivativeIndex < 0) || (derivativeIndex > 7))
				{
					display_message(WARNING_MESSAGE, "FieldML Writer:  Derivative out of range");
					return FML_INVALID_OBJECT_HANDLE;
				}
				const int versionIndex = eft->getTermNodeVersion(f, t);
				sprintf(idString, ".nodeparameters.node%d.%s.v%d", localNodeIndex + 1, derivativeNames[derivativeIndex], versionIndex + 1);
				std::string nodeParameterName = name + idString;
				FmlObjectHandle fmlNodeParameter = Fieldml_GetObjectByName(this->fmlSession, nodeParameterName.c_str());
				if (fmlNodeParameter == FML_INVALID_OBJECT_HANDLE)
				{
					fmlNodeParameter = Fieldml_CreateReferenceEvaluator(this->fmlSession, nodeParameterName.c_str(),
						fmlEftNodeParametersArgument, fmlRealType);
					if (fmlNodeParameter == FML_INVALID_OBJECT_HANDLE)
						return FML_INVALID_OBJECT_HANDLE;
					if (CMZN_OK != this->setMinimumNodeVersions(versionIndex + 1))
						return FML_INVALID_OBJECT_HANDLE;
					if ((FML_OK != Fieldml_SetBind(this->fmlSession, fmlNodeParameter, fmlEftNodesArgument, fmlEftNodeIndexConstants[localNodeIndex]))
						|| (FML_OK != Fieldml_SetBind(this->fmlSession, fmlNodeParameter, this->fmlNodeDerivativesArgument, this->fmlNodeDerivativeConstants[derivativeIndex]))
						|| (FML_OK != Fieldml_SetBind(this->fmlSession, fmlNodeParameter, this->fmlNodeVersionsArgument, this->fmlNodeVersionConstants[versionIndex])))
					{
						display_message(WARNING_MESSAGE, "FieldML Writer:  Failed to get eft node parameter evaluator");
						return FML_INVALID_OBJECT_HANDLE;
					}
				}
				fmlParameterComponents[f] = fmlNodeParameter;
			}
		}
		if (fmlParameterComponents[f] == FML_INVALID_OBJECT_HANDLE)
			return FML_INVALID_OBJECT_HANDLE;
	}

	std::string eftParametersName = name + ".parameters";
	FmlObjectHandle fmlEftParameters = Fieldml_CreateAggregateEvaluator(this->fmlSession, eftParametersName.c_str(), fmlBasisParametersType);
	if (fmlEftParameters == FML_INVALID_OBJECT_HANDLE)
		return FML_INVALID_OBJECT_HANDLE;
	if (FML_OK != (fmlError = Fieldml_SetIndexEvaluator(this->fmlSession, fmlEftParameters, /*index*/1, fmlBasisParametersComponentArgument)))
		return FML_INVALID_OBJECT_HANDLE;
	for (int f = 0; f < functionCount; ++f)
	{
		if (FML_OK != (fmlError = Fieldml_SetEvaluator(this->fmlSession, fmlEftParameters, /*element*/f + 1, fmlParameterComponents[f])))
			return FML_INVALID_OBJECT_HANDLE;
	}

	// Note the EFT keeps the basis evaluator's chart.nd argument; mesh element evaluator must bind mesh chart
	FmlObjectHandle fmlEft = Fieldml_CreateReferenceEvaluator(this->fmlSession, name.c_str(), fmlBasisEvaluator, fmlRealType);
	if (fmlEft == FML_INVALID_OBJECT_HANDLE)
		return FML_INVALID_OBJECT_HANDLE;
	if (FML_OK != (fmlError = Fieldml_SetBind(this->fmlSession, fmlEft,
			fmlBasisParametersArgument, fmlEftParameters)))
		return FML_INVALID_OBJECT_HANDLE;

	return fmlEft;
}

FmlObjectHandle FieldMLWriter::writeMeshElementEvaluator(const FE_mesh *mesh,
	const FE_mesh_element_field_template_data *meshEFTData, const std::string& eftName)
{
	if (!(mesh && meshEFTData))
		return FML_INVALID_OBJECT_HANDLE;

	FE_element_field_template *eft = meshEFTData->getElementfieldtemplate();

	FmlObjectHandle fmlEftNodesArgument, fmlEftNodeParametersArgument,
		fmlEftScaleFactorIndexesArgument, fmlEftScaleFactorsArgument;

	FmlObjectHandle fmlEft = this->writeElementfieldtemplate(eft, eftName,
		fmlEftNodesArgument, fmlEftNodeParametersArgument,
		fmlEftScaleFactorIndexesArgument, fmlEftScaleFactorsArgument);
	if (fmlEft == FML_INVALID_OBJECT_HANDLE)
		return FML_INVALID_OBJECT_HANDLE;

	FmlObjectHandle fmlEftLocalToGlobalNodes = FML_INVALID_OBJECT_HANDLE;
	const int nodeCount = eft->getNumberOfLocalNodes();
	if (nodeCount > 0)
	{
		// write local-to-global node map
		FmlObjectHandle fmlElementsType = this->fmlMeshElementsType[mesh->getDimension()];
		FmlObjectHandle fmlElementsArgument = this->getArgumentForType(fmlElementsType);
		std::string eftLocalToGlobalNodesName = eftName + ".localtoglobalnodes";
		FE_mesh_local_to_global_nodes_parameter_generator localToGlobalNodesParameterGenerator(meshEFTData, fmlElementsArgument, fmlEftNodesArgument);
		FmlObjectHandle fmlNodesType = this->fmlNodesTypes[CMZN_FIELD_DOMAIN_TYPE_NODES];
		if (localToGlobalNodesParameterGenerator.isDense())
			fmlEftLocalToGlobalNodes = this->writeDenseParameters<int>(eftLocalToGlobalNodesName, fmlNodesType, localToGlobalNodesParameterGenerator);
		else
			fmlEftLocalToGlobalNodes = this->writeSparseParameters<int>(eftLocalToGlobalNodesName, fmlNodesType, localToGlobalNodesParameterGenerator);
		if (fmlEftLocalToGlobalNodes == FML_INVALID_OBJECT_HANDLE)
		{
			display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to create element local to global nodes map");
			return CMZN_ERROR_GENERAL;
		}
	}

	const int scaleFactorCount = eft->getNumberOfLocalScaleFactors();
	if (scaleFactorCount > 0)
	{
		// write scale factors
		display_message(WARNING_MESSAGE, "FieldML Writer:  Scale factor output is not yet implemented, omitting"); // GRC TODO
	}

	FmlObjectHandle fmlRealType = this->libraryImport("real.1d");
	std::string meshElementEvaluatorName = eftName + ".evaluator";
	FmlObjectHandle fmlMeshElementEvaluator = Fieldml_CreateReferenceEvaluator(this->fmlSession, meshElementEvaluatorName.c_str(), fmlEft, fmlRealType);
	if (fmlMeshElementEvaluator == FML_INVALID_OBJECT_HANDLE)
	{
		display_message(WARNING_MESSAGE, "FieldML Writer:  Failed to create mesh element evaluator");
		return FML_INVALID_OBJECT_HANDLE;
	}
	// must bind mesh chart to generic chart used by EFT
	FmlErrorNumber fmlError;
	const int meshDimension = mesh->getDimension();
	FmlObjectHandle fmlChartArgument =
		(3 == meshDimension) ? this->libraryImport("chart.3d.argument") :
		(2 == meshDimension) ? this->libraryImport("chart.2d.argument") :
		this->libraryImport("chart.1d.argument");
	FmlObjectHandle fmlMeshType = Fieldml_GetObjectByName(this->fmlSession, eft->getMesh()->getName());
	FmlObjectHandle fmlMeshChartType = Fieldml_GetMeshChartType(this->fmlSession, fmlMeshType);
	FmlObjectHandle fmlMeshChartArgument = getArgumentForType(fmlMeshChartType);
	if (FML_OK != (fmlError = Fieldml_SetBind(this->fmlSession, fmlMeshElementEvaluator, fmlChartArgument, fmlMeshChartArgument)))
	{
		display_message(WARNING_MESSAGE, "FieldML Writer:  Failed to bind mesh chart to generic chart");
		return FML_INVALID_OBJECT_HANDLE;
	}
	if (nodeCount > 0)
	{
		if ((FML_OK != (fmlError = Fieldml_SetBind(this->fmlSession, fmlMeshElementEvaluator,
				fmlEftNodeParametersArgument, this->fmlNodesParametersArguments[CMZN_FIELD_DOMAIN_TYPE_NODES])))
			|| ((FML_OK != (fmlError = Fieldml_SetBind(this->fmlSession, fmlMeshElementEvaluator,
				this->getArgumentForType(this->fmlNodesTypes[CMZN_FIELD_DOMAIN_TYPE_NODES]), fmlEftLocalToGlobalNodes)))))
		{
			display_message(WARNING_MESSAGE, "FieldML Writer:  Failed to bind global node parameters and/or local-to-global nodes map for mesh element evaluator");
			return FML_INVALID_OBJECT_HANDLE;
		}
	}
	else
	{
		display_message(WARNING_MESSAGE, "FieldML Writer:  Only implemented for node-based mesh element evaluator");
		return FML_INVALID_OBJECT_HANDLE;
		// GRC future: bind scale factors
	}
	return fmlMeshElementEvaluator;
}

FmlObjectHandle FieldMLWriter::writeMeshfieldtemplate(const FE_mesh *mesh, const FE_mesh_field_template *mft,
	const std::string name, bool isDense, int mftEftCount, FmlObjectHandle fmlEftIndexes,
	int outputEftCount, const std::vector<int> outputEftIndexes,
	const std::vector<FmlObjectHandle> fmlMeshElementEvaluators)
{
	FmlObjectHandle fmlMft = FML_INVALID_OBJECT_HANDLE;
	FmlObjectHandle fmlRealType = this->libraryImport("real.1d");
	FmlObjectHandle fmlMeshElementsArgument = this->getArgumentForType(this->fmlMeshElementsType[mesh->getDimension()]);
	FmlErrorNumber fmlError;
	if (isDense && (mftEftCount <= 1))
	{
		// simple case of single element evaluator for all elements in mesh is written more succinctly
		fmlMft = Fieldml_CreatePiecewiseEvaluator(this->fmlSession, name.c_str(), fmlRealType);
		if (FML_OK != (fmlError = Fieldml_SetIndexEvaluator(this->fmlSession, fmlMft, /*index*/1, fmlMeshElementsArgument)))
		{
			display_message(WARNING_MESSAGE, "FieldML Writer:  Failed to set simple mesh field template index evaluator");
			return FML_INVALID_OBJECT_HANDLE;
		}
		if (mftEftCount == 1)
		{
			DsLabelIndex firstElementIndex = mesh->getLabels().getFirstIndex();
			const int eftIndex = mft->getElementEFTIndex(firstElementIndex);
			if (FML_OK != (fmlError = Fieldml_SetDefaultEvaluator(this->fmlSession, fmlMft,
				fmlMeshElementEvaluators[outputEftIndexes[eftIndex] - 1])))
			{
				display_message(WARNING_MESSAGE, "FieldML Writer:  Failed to set simple mesh field template default evaluator");
				return FML_INVALID_OBJECT_HANDLE;
			}
		}
	}
	else
	{
		// use indirect element to eft index map
		// future: can add third case for single EFT defined over part of mesh
		std::string mftEvaluatorMapName = name + ".eftmap";
		FE_mft_eft_map_parameter_generator mftEftMapParameterGenerator(mft, fmlMeshElementsArgument, isDense, outputEftCount, outputEftIndexes);
		FmlObjectHandle fmlMftEftIndexMap;
		if (mftEftMapParameterGenerator.isDense())
			fmlMftEftIndexMap = this->writeDenseParameters<int>(mftEvaluatorMapName.c_str(), fmlEftIndexes, mftEftMapParameterGenerator);
		else
			fmlMftEftIndexMap = this->writeSparseParameters<int>(mftEvaluatorMapName.c_str(), fmlEftIndexes, mftEftMapParameterGenerator);
		if (fmlMftEftIndexMap == FML_INVALID_OBJECT_HANDLE)
		{
			display_message(WARNING_MESSAGE, "FieldML Writer:  Failed to write element field template map for mesh field template");
			return FML_INVALID_OBJECT_HANDLE;
		}
		FmlObjectHandle fmlEftIndexesArgument = this->getArgumentForType(fmlEftIndexes);
		fmlMft = Fieldml_CreatePiecewiseEvaluator(this->fmlSession, name.c_str(), fmlRealType);
		if (FML_OK != (fmlError = Fieldml_SetIndexEvaluator(this->fmlSession, fmlMft, /*index*/1, fmlEftIndexesArgument)))
		{
			display_message(WARNING_MESSAGE, "FieldML Writer:  Failed to set mesh field template indirect index evaluator");
			return FML_INVALID_OBJECT_HANDLE;
		}
		if (FML_OK != (fmlError = Fieldml_SetBind(this->fmlSession, fmlMft, fmlEftIndexesArgument, fmlMftEftIndexMap)))
		{
			display_message(WARNING_MESSAGE, "FieldML Writer:  Failed to bind mesh field template indirect eft index map");
			return FML_INVALID_OBJECT_HANDLE;
		}
		for (int e = 0; e < outputEftCount; ++e)
		{
			if (mftEftMapParameterGenerator.isEftIndexUsed(e))
			{
				if (FML_OK != (fmlError = Fieldml_SetEvaluator(this->fmlSession, fmlMft, static_cast<FmlEnsembleValue>(e + 1), fmlMeshElementEvaluators[e])))
				{
					display_message(WARNING_MESSAGE, "FieldML Writer:  Failed to set mesh field template evaluator");
					return FML_INVALID_OBJECT_HANDLE;
				}
			}
		}
	}
	return fmlMft;
}

FmlObjectHandle FieldMLWriter::writeMeshField(const FE_mesh *mesh, FE_field *field,
	const std::map<const FE_mesh_field_template *, FmlObjectHandle> fmlMftsMap)
{
	// get value type
	FmlObjectHandle fmlValueType = FML_INVALID_OBJECT_HANDLE;
	FmlObjectHandle fmlComponentsType = FML_INVALID_OBJECT_HANDLE;
	FmlObjectHandle fmlComponentsArgument = FML_INVALID_OBJECT_HANDLE;
	const bool isCoordinate = field->get_CM_field_type() == CM_COORDINATE_FIELD;
	const int componentCount = get_FE_field_number_of_components(field);

	const Coordinate_system& coordinate_system = field->getCoordinateSystem();
	std::string componentsTypeName;
	if (isCoordinate && (componentCount <= 3) && (RECTANGULAR_CARTESIAN == coordinate_system.type))
	{
		if (1 == componentCount)
			fmlValueType = this->libraryImport("coordinates.rc.1d");
		else
		{
			if (2 == componentCount)
			{
				fmlValueType = this->libraryImport("coordinates.rc.2d");
				componentsTypeName = "coordinates.rc.2d.component";
				fmlComponentsType = this->libraryImport(componentsTypeName.c_str());
				fmlComponentsArgument = this->libraryImport("coordinates.rc.2d.component.argument");
			}
			else // 3-D
			{
				fmlValueType = this->libraryImport("coordinates.rc.3d");
				componentsTypeName = "coordinates.rc.3d.component";
				fmlComponentsType = this->libraryImport(componentsTypeName.c_str());
				fmlComponentsArgument = this->libraryImport("coordinates.rc.3d.component.argument");
			}
			this->typeArgument[fmlComponentsType] = fmlComponentsArgument;
		}
	}
	else
	{
		if (isCoordinate && (RECTANGULAR_CARTESIAN != coordinate_system.type))
		{
			display_message(WARNING_MESSAGE, "FieldMLWriter: Field %s written without %s coordinate system attribute(s)",
				get_FE_field_name(field), ENUMERATOR_STRING(Coordinate_system_type)(coordinate_system.type));
		}
		std::string fieldDomainName(get_FE_field_name(field));
		fieldDomainName += ".domain";
		fmlValueType = Fieldml_CreateContinuousType(this->fmlSession, fieldDomainName.c_str());
		if (1 < componentCount)
		{
			componentsTypeName = fieldDomainName + ".components";
			fmlComponentsType = Fieldml_CreateContinuousTypeComponents(
				this->fmlSession, fmlValueType, componentsTypeName.c_str(), componentCount);
			fmlComponentsArgument = this->getArgumentForType(fmlComponentsType);
		}
	}
	if ((FML_INVALID_OBJECT_HANDLE == fmlValueType) ||
		((1 < componentCount) && (FML_INVALID_OBJECT_HANDLE == fmlComponentsArgument)))
		return FML_INVALID_OBJECT_HANDLE;

	// write nodal parameters
	const DsLabels *labelsArray[4];
	int labelsArraySize = 0;
	const FE_nodeset *nodeset = FE_region_find_FE_nodeset_by_field_domain_type(this->fe_region, CMZN_FIELD_DOMAIN_TYPE_NODES);
	const DsLabels& nodeLabels = nodeset->getLabels();
	labelsArray[labelsArraySize++] = &nodeLabels;
	HDsLabels derivativesLabels;
	HDsLabels versionsLabels;
	int highestNodeDerivative = 0;
	int highestNodeVersion = 0;
	nodeset->getHighestNodeFieldDerivativeAndVersion(field, highestNodeDerivative, highestNodeVersion);
	if (highestNodeDerivative > 1)
	{
		derivativesLabels = this->nodeDerivatives;
		labelsArray[labelsArraySize++] = cmzn::GetImpl(derivativesLabels);
	}
	if (highestNodeVersion > 1)
	{
		this->setMinimumNodeVersions(highestNodeVersion);
		versionsLabels = this->nodeVersions;
		labelsArray[labelsArraySize++] = cmzn::GetImpl(versionsLabels);
	}
	// having components as the last index is typically more efficient
	// since most new meshes use the same structure for all components
	HDsLabels componentsLabels;
	if (1 < componentCount)
	{
		cmzn::SetImpl(componentsLabels, new DsLabels());
		// must set name to same as fmlComponentsType for it to be found when writing map
		componentsLabels->setName(componentsTypeName);
		componentsLabels->addLabelsRange(1, componentCount);
		labelsArray[labelsArraySize++] = cmzn::GetImpl(componentsLabels);
	}
	HDsMapDouble nodesFieldParametersMap(DsMap<double>::create(labelsArraySize, labelsArray));
	// Future: for efficiency, resize map for highest versions and derivatives before using
	std::string nodesFieldParametersMapName("nodes.");
	nodesFieldParametersMapName += get_FE_field_name(field);
	nodesFieldParametersMap->setName(nodesFieldParametersMapName);
	HDsMapIndexing nodesFieldParametersMapIndexing(nodesFieldParametersMap->createIndexing());
	HDsLabelIterator nodesLabelsIterator(nodeLabels.createLabelIterator());
	int return_code = CMZN_OK;
	// make array big enough to fit all node parameters
	const int maximumValuesCount = highestNodeDerivative*highestNodeVersion;
	std::vector<double> valuesVector(componentCount*maximumValuesCount);
	double *values = valuesVector.data();
	while (nodesLabelsIterator->increment() && (CMZN_OK == return_code))
	{
		cmzn_node *node = nodeset->findNodeByIdentifier(nodesLabelsIterator->getIdentifier());
		if (!node)
		{
			return_code = CMZN_ERROR_GENERAL;
			break;
		}
		const FE_node_field *node_field = node->getNodeField(field);
		if (!node_field)
		{
			continue; // field not defined at node
		}
		nodesFieldParametersMapIndexing->setEntry(*nodesLabelsIterator);
		if (node_field->isHomogeneousMultiComponent())
		{
			// set all components simultaneously
			nodesFieldParametersMapIndexing->setAllLabels(*componentsLabels);
			const FE_node_field_template &nft = *(node_field->getComponent(0));
			const int valueLabelsCount = nft.getValueLabelsCount();
			for (int d = 0; d < valueLabelsCount; ++d)
			{
				const cmzn_node_value_label valueLabel = nft.getValueLabelAtIndex(d);
				if (derivativesLabels)
				{
					nodesFieldParametersMapIndexing->setEntryIdentifier(*derivativesLabels, valueLabel - (CMZN_NODE_VALUE_LABEL_VALUE - 1));
				}
				const int versionsCount = nft.getVersionsCountAtIndex(d);
				for (int v = 0; v < versionsCount; ++v)
				{
					if (versionsLabels)
					{
						nodesFieldParametersMapIndexing->setEntryIdentifier(*versionsLabels, v + 1);
					}
					// following will cease to compile if FE_value is not double:
					if (CMZN_OK != get_FE_nodal_FE_value_value(node, field, /*componentNumber=all*/-1,
						valueLabel, v, /*time*/0.0, values))
					{
						return_code = CMZN_ERROR_GENERAL;
						d = valueLabelsCount;
						break;
					}
					if (!nodesFieldParametersMap->setValues(*nodesFieldParametersMapIndexing, componentCount, values))
					{
						return_code = CMZN_ERROR_GENERAL;
						d = valueLabelsCount;
						break;
					}
				}
			}
		}
		else
		{
			for (int c = 0; c < componentCount; ++c)
			{
				const FE_node_field_template &nft = *(node_field->getComponent(c));
				const int valueLabelsCount = nft.getValueLabelsCount();
				if (valueLabelsCount == 0)
				{
					continue;
				}
				nodesFieldParametersMapIndexing->setEntryIndex(*componentsLabels, c);
				// following will cease to compile if FE_value is not double:
				if (!cmzn_node_get_field_component_FE_value_values(node, field, c, /*time*/0.0, maximumValuesCount, values))
				{
					return_code = CMZN_ERROR_GENERAL;
					break;
				}
				double *value = values;
				for (int d = 0; d < valueLabelsCount; ++d)
				{
					const cmzn_node_value_label valueLabel = nft.getValueLabelAtIndex(d);
					if (derivativesLabels)
					{
						nodesFieldParametersMapIndexing->setEntryIdentifier(*derivativesLabels, valueLabel - (CMZN_NODE_VALUE_LABEL_VALUE - 1));
					}
					const int versionsCount = nft.getVersionsCountAtIndex(d);
					for (int v = 0; v < versionsCount; ++v)
					{
						if (versionsLabels)
						{
							nodesFieldParametersMapIndexing->setEntryIdentifier(*versionsLabels, v + 1);
						}
						if (!nodesFieldParametersMap->setValues(*nodesFieldParametersMapIndexing, 1, value))
						{
							return_code = CMZN_ERROR_GENERAL;
							d = valueLabelsCount;
							c = componentCount;
							break;
						}
						++value;
					}
				}
			}
		}
	}
	if (CMZN_OK != return_code)
	{
		display_message(ERROR_MESSAGE, "FieldMLWriter: Can't get nodal parameters for field %s", get_FE_field_name(field));
		return FML_INVALID_OBJECT_HANDLE;
	}
	FmlObjectHandle fmlRealType = this->libraryImport("real.1d");
	FmlObjectHandle fmlNodesFieldParameters = this->defineParametersFromMap(*nodesFieldParametersMap, fmlRealType);
	if (fmlNodesFieldParameters == FML_INVALID_OBJECT_HANDLE)
		return FML_INVALID_OBJECT_HANDLE;

	FmlObjectHandle fmlField = FML_INVALID_OBJECT_HANDLE;
	FmlErrorNumber fmlError;
	FE_mesh_field_data *meshFieldData = field->getMeshFieldData(mesh);
	const FE_mesh_field_template *mft1 = meshFieldData->getComponentMeshfieldtemplate(0);
	if (1 == componentCount)
	{
		fmlField = Fieldml_CreateReferenceEvaluator(this->fmlSession, get_FE_field_name(field), fmlMftsMap.at(mft1), fmlValueType);
	}
	else
	{
		fmlField = Fieldml_CreateAggregateEvaluator(this->fmlSession, get_FE_field_name(field), fmlValueType);
		fmlError = Fieldml_SetIndexEvaluator(this->fmlSession, fmlField, 1, fmlComponentsArgument);
		if (FML_OK != fmlError)
			return FML_INVALID_OBJECT_HANDLE;
		bool defaultEvaluator = true;
		for (int c = 1; c < componentCount; ++c)
		{
			if (meshFieldData->getComponentMeshfieldtemplate(c) != mft1)
			{
				defaultEvaluator = false;
				break;
			}
		}
		if (defaultEvaluator)
		{
			if (FML_OK != (fmlError = Fieldml_SetDefaultEvaluator(this->fmlSession, fmlField, fmlMftsMap.at(mft1))))
			{
				display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to set default evaluator for field %s", get_FE_field_name(field));
				return FML_INVALID_OBJECT_HANDLE;
			}
		}
		else
		{
			for (int c = 0; c < componentCount; ++c)
			{
				const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
				if (FML_OK != (fmlError = Fieldml_SetEvaluator(this->fmlSession, fmlField, static_cast<FmlEnsembleValue>(c + 1), fmlMftsMap.at(mft))))
				{
					display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to set component mesh field template for field %s", get_FE_field_name(field));
					return FML_INVALID_OBJECT_HANDLE;
				}
			}
		}
	}
	if (FML_OK != (fmlError = Fieldml_SetBind(this->fmlSession, fmlField,
		this->fmlNodesParametersArguments[CMZN_FIELD_DOMAIN_TYPE_NODES], fmlNodesFieldParameters)))
	{
		display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to bind node parameters for field %s", get_FE_field_name(field));
		return FML_INVALID_OBJECT_HANDLE;
	}
	// future: bind scale factors
	return fmlField;
}

int FieldMLWriter::writeMeshFields(int meshDimension)
{
	FE_mesh *mesh = FE_region_find_FE_mesh_by_dimension(this->fe_region, meshDimension);
	// get list of finite element fields to write, in default order
	std::vector<FE_field *> fields;
	// get list of mesh field templates to write, in order of field, component
	std::vector<const FE_mesh_field_template *> mfts;
	cmzn_fielditerator_id cfieldIter = this->fe_region->create_fielditerator();
	if (!cfieldIter)
		return CMZN_ERROR_MEMORY;
	cmzn_field_id cfield;
	while (0 != (cfield = cmzn_fielditerator_next_non_access(cfieldIter)))
	{
		FE_field *field = 0;
		if (Computed_field_get_type_finite_element(cfield, &field) && field)
		{
			if ((field->get_FE_field_type() == GENERAL_FE_FIELD)
				&& (get_FE_field_value_type(field) == FE_VALUE_VALUE))
			{
				FE_mesh_field_data *meshFieldData = field->getMeshFieldData(mesh);
				if (meshFieldData) // i.e. is field defined on mesh
				{
					fields.push_back(field);
					const int componentCount = get_FE_field_number_of_components(field);
					for (int c = 0; c < componentCount; ++c)
					{
						const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
						size_t m;
						for (m = 0; m < mfts.size(); ++m)
						{
							if (mfts[m] == mft)
								break;
						}
						if (m == mfts.size())
							mfts.push_back(mft);
					}
				}
			}
			else
				display_message(WARNING_MESSAGE, "FieldMLWriter: Cannot write finite element field %s"
					" because it is not real-valued with standard interpolation.", get_FE_field_name(field));
		}
	}
	cmzn_fielditerator_destroy(&cfieldIter);

	std::string meshName(mesh->getName());
	const DsLabels& elementLabels = mesh->getLabels();

	// get list of element field templates to write, in order of mfts, element:
	std::vector<const FE_element_field_template *> efts;
	// determine whether mft maps all elements of mesh; can use dense map or no map if 1 EFT
	std::vector<bool> mftIsDense(mfts.size(), false);
	// get number of EFTs each MFT uses; can optimise output if 1
	std::vector<int> mftEftCounts(mfts.size(), 0);
	// get map from internal EFT index to output index (starting at 1)
	std::vector<int> outputEftIndexes(mesh->getElementfieldtemplateDataCount(), 0);
	auto eftCount = efts.size();
	for (size_t m = 0; m < mfts.size(); ++m)
	{
		const FE_mesh_field_template *mft = mfts[m];
		std::vector<bool> eftsUsed(eftCount, false);
		DsLabelIterator *iter = elementLabels.createLabelIterator();
		DsLabelIndex elementIndex;
		bool isDense = true;
		const FE_element_field_template *lastEFT = 0;
		while (DS_LABEL_IDENTIFIER_INVALID != (elementIndex = iter->nextIndex()))
		{
			const FE_element_field_template *eft = mft->getElementfieldtemplate(elementIndex);
			if (eft)
			{
				if (eft != lastEFT)
				{
					size_t e;
					for (e = 0; e < eftCount; ++e)
					{
						if (efts[e] == eft)
						{
							eftsUsed[e] = true;
							break;
						}
					}
					if (e == eftCount)
					{
						efts.push_back(eft);
						eftsUsed.push_back(true);
						eftCount = efts.size();
						outputEftIndexes[eft->getIndexInMesh()] = static_cast<int>(eftCount);
					}
					lastEFT = eft;
				}
			}
			else
				isDense = false;
		}
		mftIsDense[m] = isDense;
		for (size_t e = 0; e < eftCount; ++e)
		{
			if (eftsUsed[e])
				++(mftEftCounts[m]);
		}
		cmzn::Deaccess(iter);
	}

	// Create set of indexes for EFTs, so MFTs can map to them
	FmlObjectHandle fmlEftIndexes = FML_INVALID_OBJECT_HANDLE;
	FmlErrorNumber fmlError;
	if (efts.size() > 0)
	{
		std::string eftIndexName = meshName + ".eftIndexes";
		fmlEftIndexes = Fieldml_CreateEnsembleType(this->fmlSession, eftIndexName.c_str());
		if (FML_OK != (fmlError = Fieldml_SetEnsembleMembersRange(this->fmlSession, fmlEftIndexes, 1, static_cast<int>(efts.size()), 1)))
		{
			display_message(ERROR_MESSAGE, "FieldMLWriter:  Failed to create mesh EFT indexes ensemble");
			return CMZN_ERROR_MEMORY;
		}
		this->getArgumentForType(fmlEftIndexes);
	}

	// write element field templates and associated local-to-global maps
	std::vector<FmlObjectHandle> fmlMeshElementEvaluators(efts.size(), FML_INVALID_OBJECT_HANDLE);
	char idString[30];
	const int eftsSize = static_cast<int>(efts.size());
	for (int e = 0; e < eftsSize; ++e)
	{
		sprintf(idString, ".eft%d", e + 1);
		FE_mesh_element_field_template_data *meshEFTData = mesh->getElementfieldtemplateData(efts[e]);
		fmlMeshElementEvaluators[e] = this->writeMeshElementEvaluator(mesh, meshEFTData, meshName + idString);
		if (FML_INVALID_OBJECT_HANDLE == fmlMeshElementEvaluators[e])
		{
			display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to write mesh element field template data");
			return CMZN_ERROR_GENERAL;
		}
	}

	// write mesh field templates
	std::map<const FE_mesh_field_template *, FmlObjectHandle> fmlMftsMap;
	const int mftsSize = static_cast<int>(mfts.size());
	for (int m = 0; m < mftsSize; ++m)
	{
		sprintf(idString, ".fieldtemplate%d", m + 1);
		FmlObjectHandle fmlMft = this->writeMeshfieldtemplate(mesh, mfts[m], meshName + idString,
			mftIsDense[m], mftEftCounts[m], fmlEftIndexes,
			static_cast<int>(efts.size()), outputEftIndexes, fmlMeshElementEvaluators);
		if (FML_INVALID_OBJECT_HANDLE == fmlMft)
		{
			display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to write mesh field template");
			return CMZN_ERROR_GENERAL;
		}
		fmlMftsMap[mfts[m]] = fmlMft;
	}

	// write fields
	for (size_t f = 0; f < fields.size(); ++f)
	{
		FmlObjectHandle fmlField = this->writeMeshField(mesh, fields[f], fmlMftsMap);
		if (FML_INVALID_OBJECT_HANDLE == fmlField)
		{
			display_message(ERROR_MESSAGE, "FieldML Writer:  Failed to write mesh field");
			return CMZN_ERROR_GENERAL;
		}
	}
	return CMZN_OK;
}

int FieldMLWriter::writeFile(const char *pathandfilename)
{
	FmlErrorNumber fmlError = Fieldml_WriteFile(this->fmlSession, pathandfilename);
	if (FML_OK == fmlError)
		return CMZN_OK;
	return CMZN_ERROR_GENERAL;
}

int write_fieldml_file(struct cmzn_region *region, const char *pathandfilename)
{
	int return_code = CMZN_OK;
	if (region && pathandfilename && (*pathandfilename != '\0'))
	{
		char *location = duplicate_string(pathandfilename);
		char *lastDirSep = strrchr(location, '/');
		char *lastDirSepWin = strrchr(location, '\\');
		if (lastDirSepWin > lastDirSep)
			lastDirSep = lastDirSepWin;
		const char *filename;
		if (lastDirSep)
		{
			*lastDirSep = '\0';
			filename = lastDirSep + 1;
		}
		else
		{
			location[0] = '\0';
			filename = pathandfilename;
		}
		FieldMLWriter fmlWriter(region, location, filename);
		if (CMZN_OK == return_code)
			return_code = fmlWriter.writeNodesets();
		// Currently only writes highest dimension mesh
		int highestMeshDimension = fmlWriter.getHighestMeshDimension();
		if (0 < highestMeshDimension)
		{
			if (CMZN_OK == return_code)
				return_code = fmlWriter.writeMesh(highestMeshDimension, /*writeIfEmpty*/false);
			if (CMZN_OK == return_code)
				return_code = fmlWriter.writeMeshFields(highestMeshDimension);
		}
		if (CMZN_OK == return_code)
			return_code = fmlWriter.writeFile(pathandfilename);
	}
	else
		return_code = CMZN_ERROR_ARGUMENT;
	return return_code;
}
