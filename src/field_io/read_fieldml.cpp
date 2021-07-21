/**
 * FILE : read_fieldml.cpp
 * 
 * FieldML 0.5 model reader implementation.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/elementbasis.h"
#include "opencmiss/zinc/elementtemplate.h"
#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/fieldcache.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/fieldfiniteelement.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/node.h"
#include "opencmiss/zinc/nodeset.h"
#include "opencmiss/zinc/nodetemplate.h"
#include "opencmiss/zinc/region.h"
#include "opencmiss/zinc/status.h"
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_finite_element.h"
#include "field_io/fieldml_common.hpp"
#include "field_io/read_fieldml.hpp"
#include "finite_element/element_field_template.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_region.h"
#include "general/block_array.hpp"
#include "general/debug.h"
#include "general/mystring.h"
#include "general/message.h"
#include "general/refcounted.hpp"
#include "mesh/cmiss_element_private.hpp"
#include "mesh/cmiss_node_private.hpp"
#include "FieldmlIoApi.h"

namespace {

const char *libraryChartArgumentNames[] =
{
	0,
	"chart.1d.argument",
	"chart.2d.argument",
	"chart.3d.argument"
};

class FieldmlEft
{
	FE_element_field_template *eft;
	// use following if node-based:
	FmlObjectHandle fmlChartArgument;
	FmlObjectHandle fmlLocalNodes;
	FmlObjectHandle fmlLocalNodesArgument;
	FmlObjectHandle fmlNodeParametersArgument;
	FmlObjectHandle fmlLocalScaleFactorsArgument;
	int access_count;

	FieldmlEft(FE_element_field_template *eftIn) :
		eft(eftIn->access()),
		fmlChartArgument(FML_INVALID_OBJECT_HANDLE),
		fmlLocalNodes(FML_INVALID_OBJECT_HANDLE),
		fmlLocalNodesArgument(FML_INVALID_OBJECT_HANDLE),
		fmlNodeParametersArgument(FML_INVALID_OBJECT_HANDLE),
		fmlLocalScaleFactorsArgument(FML_INVALID_OBJECT_HANDLE),
		access_count(1)
	{
	}

	~FieldmlEft()
	{
		FE_element_field_template::deaccess(this->eft);
	}

public:
	static FieldmlEft* create(FE_element_field_template *eftIn)
	{
		if (eftIn)
			return new FieldmlEft(eftIn);
		return 0;
	}

	FieldmlEft* access()
	{
		++this->access_count;
		return this;
	}

	static void deaccess(FieldmlEft* &fieldmlEft)
	{
		if (fieldmlEft)
		{
			--(fieldmlEft->access_count);
			if (fieldmlEft->access_count <= 0)
				delete fieldmlEft;
			fieldmlEft = 0;
		}
	}

	FE_element_field_template *getEft() const
	{
		return this->eft;
	}

	FmlObjectHandle getFmlChartArgument() const
	{
		return this->fmlChartArgument;
	}

	void setFmlChartArgument(FmlObjectHandle fmlChartArgumentIn)
	{
		this->fmlChartArgument = fmlChartArgumentIn;
	}

	FmlObjectHandle getFmlLocalNodes() const
	{
		return this->fmlLocalNodes;
	}

	void setFmlLocalNodes(FmlObjectHandle fmlLocalNodesIn)
	{
		this->fmlLocalNodes = fmlLocalNodesIn;
	}

	FmlObjectHandle getFmlLocalNodesArgument() const
	{
		return this->fmlLocalNodesArgument;
	}

	void setFmlLocalNodesArgument(FmlObjectHandle fmlLocalNodesArgumentIn)
	{
		this->fmlLocalNodesArgument = fmlLocalNodesArgumentIn;
	}

	FmlObjectHandle getFmlNodeParametersArgument() const
	{
		return this->fmlNodeParametersArgument;
	}

	void setFmlNodeParametersArgument(FmlObjectHandle fmlNodeParametersArgumentIn)
	{
		this->fmlNodeParametersArgument = fmlNodeParametersArgumentIn;
	}

	FmlObjectHandle getFmlLocalScaleFactorsArgument() const
	{
		return this->fmlLocalScaleFactorsArgument;
	}

	void setFmlLocalScaleFactorsArgument(FmlObjectHandle fmlLocalScaleFactorsArgumentIn)
	{
		this->fmlLocalScaleFactorsArgument = fmlLocalScaleFactorsArgumentIn;
	}

	/** Validate, lock and merge element field template into mesh.
	  * @return  True on success, false if failed. */
	bool validateAndMergeEftIntoMesh()
	{
		if (!this->eft->validateAndLock())
			return false;
		FE_element_field_template *mergedEft = this->eft->getMesh()->mergeElementfieldtemplate(this->eft);
		if (!mergedEft)
			return false;
		FE_element_field_template::deaccess(this->eft);
		this->eft = mergedEft; // take over access count
		return true;
	}

};

class MeshElementEvaluator
{
	FieldmlEft *fieldmlEft;
	// use following if node-based:
	FmlObjectHandle fmlLocalToGlobalNodeMap;

public:

	MeshElementEvaluator(FieldmlEft *fieldmlEftIn) :
		fieldmlEft(fieldmlEftIn->access()),
		fmlLocalToGlobalNodeMap(FML_INVALID_OBJECT_HANDLE)
	{
	}

	~MeshElementEvaluator()
	{
		FieldmlEft::deaccess(this->fieldmlEft);
	}

	FE_element_field_template *getEft() const
	{
		return this->fieldmlEft->getEft();
	}

	FmlObjectHandle getFmlLocalToGlobalNodeMap() const
	{
		return this->fmlLocalToGlobalNodeMap;
	}

	void setFmlLocalToGlobalNodeMap(FmlObjectHandle fmlLocalToGlobalNodeMapIn)
	{
		this->fmlLocalToGlobalNodeMap = fmlLocalToGlobalNodeMapIn;
	}

};

typedef std::map<FmlObjectHandle, MeshElementEvaluator*> MeshElementEvaluatorMap;
typedef std::map<FmlObjectHandle, FE_mesh_field_template*> MeshfieldtemplateMap;
typedef std::map<FmlObjectHandle, HDsLabels> FmlObjectLabelsMap;
typedef std::map<FmlObjectHandle, HDsMapInt> FmlObjectIntParametersMap;
typedef std::map<FmlObjectHandle, HDsMapDouble> FmlObjectDoubleParametersMap;

class FieldMLReader
{
	template <typename VALUETYPE> friend class DsMapParameterConsumer;

	cmzn_region *region;
	FE_mesh *mesh;
	cmzn_fieldmodule_id field_module;
	const char *filename;
	FmlSessionHandle fmlSession;
	FmlObjectHandle fmlNodesType;
	FmlObjectHandle fmlNodesArgument;
	FmlObjectHandle fmlMeshType;
	FmlObjectHandle fmlMeshArgument;
	FmlObjectHandle fmlMeshChartArgument;
	FmlObjectHandle fmlElementsType;
	FmlObjectHandle fmlElementsArgument;
	FmlObjectHandle fmlNodeParametersArgument;
	FmlObjectHandle fmlNodeDerivativesType;
	FmlObjectHandle fmlNodeDerivativesArgument;
	FmlObjectHandle fmlNodeVersionsType;
	FmlObjectHandle fmlNodeVersionsArgument;
	MeshElementEvaluatorMap meshElementEvaluatorMap;
	MeshfieldtemplateMap meshfieldtemplateMap;
	FmlObjectLabelsMap labelsMap;
	FmlObjectIntParametersMap intParametersMap;
	FmlObjectDoubleParametersMap doubleParametersMap;
	bool verbose;
	int nameBufferLength;
	char *nameBuffer; // buffer for reading object names into
	std::set<FmlObjectHandle> processedObjects;

public:
	FieldMLReader(struct cmzn_region *region, const char *filename) :
		region(cmzn_region_access(region)),
		mesh(0),
		field_module(cmzn_region_get_fieldmodule(region)),
		filename(filename),
		fmlSession(Fieldml_CreateFromFile(filename)),
		fmlNodesType(FML_INVALID_OBJECT_HANDLE),
		fmlNodesArgument(FML_INVALID_OBJECT_HANDLE),
		fmlMeshType(FML_INVALID_OBJECT_HANDLE),
		fmlMeshArgument(FML_INVALID_OBJECT_HANDLE),
		fmlMeshChartArgument(FML_INVALID_OBJECT_HANDLE),
		fmlElementsType(FML_INVALID_OBJECT_HANDLE),
		fmlElementsArgument(FML_INVALID_OBJECT_HANDLE),
		fmlNodeParametersArgument(FML_INVALID_OBJECT_HANDLE),
		fmlNodeDerivativesType(FML_INVALID_OBJECT_HANDLE),
		fmlNodeDerivativesArgument(FML_INVALID_OBJECT_HANDLE),
		fmlNodeVersionsType(FML_INVALID_OBJECT_HANDLE),
		fmlNodeVersionsArgument(FML_INVALID_OBJECT_HANDLE),
		verbose(false),
		nameBufferLength(50),
		nameBuffer(new char[nameBufferLength])
	{
		Fieldml_SetDebug(fmlSession, /*debug*/verbose);
	}

	~FieldMLReader()
	{
		for (auto iter = this->meshElementEvaluatorMap.begin(); iter != meshElementEvaluatorMap.end(); ++iter)
			delete (iter->second);
		for (auto iter = this->meshfieldtemplateMap.begin(); iter != meshfieldtemplateMap.end(); ++iter)
			FE_mesh_field_template::deaccess(iter->second);
		Fieldml_Destroy(fmlSession);
		cmzn_fieldmodule_destroy(&field_module);
		cmzn_region_destroy(&region);
		delete[] nameBuffer;
	}

	/** @return  1 on success, 0 on failure */
	int parse();

private:

	std::string getName(FmlObjectHandle fmlObjectHandle)
	{
		if (FML_INVALID_HANDLE == fmlObjectHandle)
			return std::string("INVALID");
		nameBuffer[0] = 0;
		while (true)
		{
			int length = Fieldml_CopyObjectName(fmlSession, fmlObjectHandle, nameBuffer, nameBufferLength);
			if (length < nameBufferLength - 1)
				break;
			nameBufferLength *= 2;
			delete[] nameBuffer;
			nameBuffer = new char[nameBufferLength];
		}
		return std::string(nameBuffer);
	}

	std::string getDeclaredName(FmlObjectHandle fmlObjectHandle)
	{
		if (FML_INVALID_HANDLE == fmlObjectHandle)
			return std::string("INVALID");
		nameBuffer[0] = 0;
		while (true)
		{
			int length = Fieldml_CopyObjectDeclaredName(fmlSession, fmlObjectHandle, nameBuffer, nameBufferLength);
			if (length < nameBufferLength - 1)
				break;
			nameBufferLength *= 2;
			delete[] nameBuffer;
			nameBuffer = new char[nameBufferLength];
		}
		return std::string(nameBuffer);
	}

	bool isValueTypeReal1D(FmlObjectHandle fmlEvaluator);

	/** @return  Index >= 1, or 0 if failed. */
	int readConstantIndex(FmlObjectHandle fmlEnsembleType, FmlObjectHandle fmlConstantEvaluator);

	DsLabels *getLabelsForEnsemble(FmlObjectHandle fmlEnsemble);

	/** @return  True on success, false on failure */
	template <typename VALUETYPE, class PARAMETERCONSUMER> bool readParameters(FmlObjectHandle fmlParameters,
		PARAMETERCONSUMER& parameterConsumer);

	DsMap<int> *getEnsembleParameters(FmlObjectHandle fmlParameters);
	DsMap<double> *getContinuousParameters(FmlObjectHandle fmlParameters);

	int readNodes(FmlObjectHandle fmlNodesArgument);

	/** find and read global nodes, derivatives and versions types */
	int readGlobals();

	FmlObjectHandle getArgumentForType(FmlObjectHandle fmlValueType);

	int readMeshes();

	FE_basis *readBasisInterpolator(FmlObjectHandle fmlInterpolator, FmlObjectHandle& fmlChartArgument,
		FmlObjectHandle& fmlParametersArgument, const int*& swizzle);

	/** Reads parameter mapping expression for the given function.
	  * If not yet known, discovers the local nodes and scale factors ensemble arguments,
	  * or if already known checks consistent usage.
	  * @param fieldmlEft  The FieldML eft to read a parameter expression for.
	  * @param functionNumber  The function number the parameter is for, starting at 0.
	  * @param fmlParameterEvaluator  The fieldml object describing the parameter map.
	  * @return  True on success, false on failure */
	bool readElementFieldTemplateParameter(FieldmlEft &fieldmlEft,
		int functionNumber, FmlObjectHandle fmlParameterEvaluator);

	/** @return  On success, FieldMLEft including locked, merged in mesh EFT, On failure, 0. */
	FieldmlEft *readElementFieldTemplate(FmlObjectHandle fmlEvaluator);

	bool isValidNodeParametersArgument(FmlObjectHandle fmlArgument);

	bool isValidLocalToGlobalNodeMap(FmlObjectHandle fmlLocalToGlobalNodeMap, FmlObjectHandle fmlLocalNodesArgument);

	/** @return  Mesh element evaluator for fmlEvaluator, or 0 if none. Not accessed. */
	inline MeshElementEvaluator* getMeshElementEvaluator(FmlObjectHandle fmlEvaluator)
	{
		auto iter = this->meshElementEvaluatorMap.find(fmlEvaluator);
		if (iter != this->meshElementEvaluatorMap.end())
			return iter->second;
		return 0;
	}

	MeshElementEvaluator* readMeshElementEvaluatorLegacy(FmlObjectHandle fmlEvaluator);

	MeshElementEvaluator* readMeshElementEvaluatorEft(FmlObjectHandle fmlEvaluator);

	/** Defines a new mesh element evaluator from fmlEvaluator, and reads local to global
	  * nodes, eventually scale factor information etc.
	  * Call getMeshElementEvaluator first.
	  * @return  Mesh element evaluator for fmlEvaluator, or 0 if failed. Not accessed. */
	MeshElementEvaluator* readMeshElementEvaluator(FmlObjectHandle fmlEvaluator);

	/** Find or read mesh field template from fmlEvaluator
	  * @return Scalar mesh field template merged in mesh, or 0 if failed. Not accessed. */
	FE_mesh_field_template *readMeshFieldTemplate(FmlObjectHandle fmlEvaluator);

	int readField(FmlObjectHandle fmlFieldEvaluator, FmlObjectHandle fmlComponentsType,
		std::vector<FmlObjectHandle> &fmlComponentEvaluators, FmlObjectHandle fmlNodeParameters,
		FmlObjectHandle fmlNodeParametersArgument, FmlObjectHandle fmlNodesArgument,
		FmlObjectHandle fmlElementArgument);

	bool evaluatorIsScalarContinuousPiecewiseOverElements(FmlObjectHandle fmlEvaluator,
		FmlObjectHandle &fmlElementArgument);

	bool evaluatorIsNodeParameters(FmlObjectHandle fmlNodeParameters, FmlObjectHandle fmlComponentsArgument);

	int readAggregateFields();

	int readReferenceFields();

	bool isProcessed(FmlObjectHandle fmlObjectHandle)
	{
		return (processedObjects.find(fmlObjectHandle) != processedObjects.end());
	}

	void setProcessed(FmlObjectHandle fmlObjectHandle)
	{
		processedObjects.insert(fmlObjectHandle);
	}

};

template <typename VALUETYPE> class DsMapParameterConsumer
{
	FieldMLReader& fieldmlReader;
	DsMap<VALUETYPE>& map;
	bool isDense;
	std::vector<FmlObjectHandle> fmlDenseIndexEvaluators;
	std::vector<HDsLabels> denseIndexLabels;
	std::vector<FmlObjectHandle> fmlSparseIndexEvaluators;
	std::vector<HDsLabels> sparseIndexLabels;
	int sparseRecordCount;
	HDsMapIndexing indexing;
	int recordCount;
	std::vector<int> denseRecordOffsets;
	std::vector<int> denseRecordSizes;
	DsLabelIndex denseIndex; // iterates in declaration order
	int denseIndexCount;
	int denseRecordBufferSize;
	int sparseIndexCount;
	int sparseRecordOffsets[2];
	int *denseRecordOffsetsPtr;
	int sparseRecordSizes[2];

public:
	DsMapParameterConsumer(FieldMLReader& fieldmlReaderIn, DsMap<VALUETYPE>& mapIn) :
		fieldmlReader(fieldmlReaderIn),
		map(mapIn),
		isDense(false),
		sparseRecordCount(0),
		recordCount(0),
		denseRecordOffsets((mapIn.getLabelsArraySize() > 2) ? mapIn.getLabelsArraySize() : 2),
		denseRecordSizes(this->denseRecordOffsets.size()),
		denseIndex(-1),
		denseIndexCount(0),
		denseRecordBufferSize(0),
		sparseIndexCount(0),
		denseRecordOffsetsPtr(this->denseRecordOffsets.data())
	{
	}

	~DsMapParameterConsumer()
	{
	}

	bool setIndexing(std::vector<FmlObjectHandle>& fmlSparseIndexEvaluatorsIn, int sparseRecordCountIn, std::vector<FmlObjectHandle>& fmlDenseIndexEvaluatorsIn)
	{
		for (auto iter = fmlSparseIndexEvaluatorsIn.begin(); iter != fmlSparseIndexEvaluatorsIn.end(); ++iter)
		{
			FmlObjectHandle fmlSparseIndexType = Fieldml_GetValueType(this->fieldmlReader.fmlSession, *iter);
			HDsLabels hLabels(this->fieldmlReader.getLabelsForEnsemble(fmlSparseIndexType));
			if (!hLabels)
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  Parameters sparse index type %s could not be read as labels.",
					this->fieldmlReader.getName(fmlSparseIndexType).c_str());
				return false;
			}
			this->sparseIndexLabels.push_back(hLabels);
		}
		this->fmlSparseIndexEvaluators = fmlSparseIndexEvaluatorsIn;
		this->sparseRecordCount = sparseRecordCountIn;
		for (auto iter = fmlDenseIndexEvaluatorsIn.begin(); iter != fmlDenseIndexEvaluatorsIn.end(); ++iter)
		{
			FmlObjectHandle fmlDenseIndexType = Fieldml_GetValueType(this->fieldmlReader.fmlSession, *iter);
			HDsLabels hLabels(this->fieldmlReader.getLabelsForEnsemble(fmlDenseIndexType));
			if (!hLabels)
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  Parameters dense index type %s could not be read as labels.",
					this->fieldmlReader.getName(fmlDenseIndexType).c_str());
				return false;
			}
			this->denseIndexLabels.push_back(hLabels);
		}
		this->fmlDenseIndexEvaluators = fmlDenseIndexEvaluatorsIn;

		HDsMapIndexing mapIndexing(this->map.createIndexing());
		if (!mapIndexing)
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Failed to create map indexing.");
			return false;
		}
		this->indexing = mapIndexing;

		this->denseIndexCount = static_cast<int>(this->denseIndexLabels.size());
		this->sparseIndexCount = static_cast<int>(this->sparseIndexLabels.size());
		this->isDense = this->sparseIndexCount == 0;
		this->sparseRecordSizes[0] = 1;
		this->sparseRecordSizes[1] = static_cast<int>(fmlSparseIndexEvaluatorsIn.size());
		this->sparseRecordOffsets[0] = -1;
		this->sparseRecordOffsets[1] = 0;
		this->denseRecordOffsets[0] = -1;
		this->denseRecordBufferSize = 1;
		if (this->isDense)
		{
			// iterate over first ensemble as assume it's a big set like nodes, elements
			this->denseIndex = -1;
			this->recordCount = (0 < this->denseIndexCount) ? this->denseIndexLabels[0]->getSize() : 1;
			this->denseRecordSizes[0] = 1;
			for (int i = 1; i < this->denseIndexCount; ++i)
			{
				this->denseRecordSizes[i] = this->denseIndexLabels[i]->getSize();
				this->denseRecordBufferSize *= this->denseIndexLabels[i]->getSize();
				this->denseRecordOffsets[i] = 0;
			}
		}
		else
		{
			// sparse format is always rank 2; first size is record count, second is product of all dense index sizes
			this->recordCount = this->sparseRecordCount;
			this->denseRecordSizes[0] = 1;
			for (int i = 0; i < this->denseIndexCount; ++i)
				this->denseRecordBufferSize *= this->denseIndexLabels[i]->getSize();
			this->denseRecordSizes[1] = this->denseRecordBufferSize;
			this->denseRecordOffsets[1] = 0;
		}
		if (this->recordCount < 0)
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Invalid number of records for element local-to-global node map parameters");
			return false;
		}
		return true;
	}

	/** Only call after setIndexing() */
	int getRecordCount() const
	{
		return this->recordCount;
	}

	bool nextRecord()
	{
		++(this->denseRecordOffsetsPtr[0]);
		if (this->isDense)
		{
			++this->denseIndex;
		}
		else
		{
			++this->sparseRecordOffsets[0];
		}
		return true;
	}

	int getDenseRecordBufferSize() const
	{
		return this->denseRecordBufferSize;
	}

	const int *getDenseRecordOffsets() const
	{
		return this->denseRecordOffsetsPtr;
	}

	const int *getDenseRecordSizes() const
	{
		return this->denseRecordSizes.data();
	}

	const int *getSparseRecordOffsets() const
	{
		return this->sparseRecordOffsets;
	}

	const int *getSparseRecordSizes() const
	{
		return this->sparseRecordSizes;
	}

	bool setDenseValues(VALUETYPE *valueBuffer)
	{
		if (0 < this->denseIndexCount)
		{
			// sanity check identifier exists
			const DsLabelIdentifier identifier = this->denseIndexLabels[0]->getIdentifier(this->denseIndex);
			if (identifier == DS_LABEL_IDENTIFIER_INVALID)
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  DsMapParameterConsumer found no label at dense index");
				return false;
			}
			if (!this->indexing->setEntryIndex(*this->denseIndexLabels[0], this->denseIndex))
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  DsMapParameterConsumer failed to set dense entry index");
				return false;
			}
		}
		if (!this->map.setValues((*this->indexing), this->denseRecordBufferSize, valueBuffer))
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  DsMapParameterConsumer failed to set dense values");
			return false;
		}
		return true;
	}

	bool setSparseValues(int *keyBuffer, VALUETYPE *valueBuffer)
	{
		for (int i = 0; i < this->sparseIndexCount; ++i)
		{
			if (!indexing->setEntryIdentifier(*(sparseIndexLabels[i]), keyBuffer[i]))
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  DsMapParameterConsumer failed to set sparse entry index");
				return false;
			}
		}
		if (!this->map.setValues((*this->indexing), this->denseRecordBufferSize, valueBuffer))
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  DsMapParameterConsumer failed to set sparse values");
			return false;
		}
		return true;
	}
};


class MeshLocalToGlobalNodeParameterConsumer
{
	FE_mesh_element_field_template_data& meshEftData;
	FE_element_field_template *eft;
	const int localNodeCount;
	std::vector<DsLabelIndex> nodeIndexesVector;
	DsLabelIndex *nodeIndexes;
	FE_mesh *mesh;
	FE_nodeset *nodeset;
	bool isDense;
	std::vector<FmlObjectHandle> fmlDenseIndexEvaluators;
	std::vector<FmlObjectHandle> fmlSparseIndexEvaluators;
	int sparseRecordCount;
	int recordCount;
	int recordOffsets[2];
	int denseRecordSizes[2];
	int sparseRecordSizes[2];
	// for dense parameters only:
	DsLabelIndex elementIndex;
	DsLabelIdentifier elementIdentifier;

public:
	MeshLocalToGlobalNodeParameterConsumer(FE_mesh_element_field_template_data& meshEftDataIn) :
		meshEftData(meshEftDataIn),
		eft(meshEftDataIn.getElementfieldtemplate()),
		localNodeCount(this->eft->getNumberOfLocalNodes()),
		nodeIndexesVector(this->localNodeCount, DS_LABEL_INDEX_INVALID),
		nodeIndexes(this->nodeIndexesVector.data()),
		mesh(this->eft->getMesh()),
		nodeset(this->mesh->getNodeset()),
		isDense(false),
		sparseRecordCount(0),
		recordCount(0),
		elementIndex(-1)
	{
		this->denseRecordSizes[0] = 1;
		this->denseRecordSizes[1] = this->localNodeCount;
		this->recordOffsets[0] = -1;
		this->recordOffsets[1] = 0;
	}

	/** Must be called before all other methods to set up indexing */
	bool setIndexing(std::vector<FmlObjectHandle>& fmlSparseIndexEvaluatorsIn, int sparseRecordCountIn, std::vector<FmlObjectHandle>& fmlDenseIndexEvaluatorsIn)
	{
		if ((fmlSparseIndexEvaluatorsIn.size() > 1) || ((fmlSparseIndexEvaluatorsIn.size() + fmlDenseIndexEvaluatorsIn.size()) != 2))
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Invalid numbers of indexes for element local-to-global node map parameters");
			return false;
		}
		this->fmlSparseIndexEvaluators = fmlSparseIndexEvaluatorsIn;
		this->sparseRecordCount = sparseRecordCountIn;
		this->fmlDenseIndexEvaluators = fmlDenseIndexEvaluatorsIn;
		this->isDense = fmlSparseIndexEvaluatorsIn.size() == 0;
		this->recordCount = (this->isDense) ? this->mesh->getSize() : this->sparseRecordCount;
		this->sparseRecordSizes[0] = 1;
		this->sparseRecordSizes[1] = static_cast<int>(fmlSparseIndexEvaluatorsIn.size());
		if (this->recordCount < 0)
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Invalid number of records for element local-to-global node map parameters");
			return false;
		}
		return true;
	}

	/** Only call after setIndexing() */
	int getRecordCount() const
	{
		return this->recordCount;
	}

	bool nextRecord()
	{
		++this->recordOffsets[0];
		if (this->isDense)
		{
			++this->elementIndex;
			this->elementIdentifier = this->mesh->getElementIdentifier(this->elementIndex);
			if (this->elementIdentifier == DS_LABEL_IDENTIFIER_INVALID)
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  MeshLocalToGlobalNodeParameterConsumer::nextRecord() found no element at index");
				return false;
			}
		}
		return true;
	}

	int getDenseRecordBufferSize() const
	{
		return this->localNodeCount;
	}

	const int *getDenseRecordOffsets() const
	{
		return this->recordOffsets;
	}

	const int *getDenseRecordSizes() const
	{
		return this->denseRecordSizes;
	}

	const int *getSparseRecordOffsets() const
	{
		return this->recordOffsets;
	}

	const int *getSparseRecordSizes() const
	{
		return this->sparseRecordSizes;
	}

	/** @param valueBuffer  Array of node identifiers, values of -1 mean no node set at index */
	bool setDenseValues(int *valueBuffer)
	{
		for (int n = 0; n < this->localNodeCount; ++n)
		{
			const DsLabelIdentifier nodeIdentifier = static_cast<DsLabelIdentifier>(valueBuffer[n]);
			if (nodeIdentifier == -1)
			{
				this->nodeIndexes[n] = DS_LABEL_INDEX_INVALID;
			}
			else
			{
				this->nodeIndexes[n] = this->nodeset->findIndexByIdentifier(nodeIdentifier);
				if (this->nodeIndexes[n] == DS_LABEL_INDEX_INVALID)
				{
					display_message(ERROR_MESSAGE, "FieldML Reader:  Could not find node %d referenced in element local-to-global nodes map",
						nodeIdentifier);
					return false;
				}
			}
		}
		if (CMZN_OK != this->meshEftData.setElementLocalNodes(this->elementIndex, this->nodeIndexes))
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  MeshLocalToGlobalNodeParameterConsumer failed to set local nodes for %d-D element %d",
				this->mesh->getDimension(), this->elementIdentifier);
			return false;
		}
		return true;
	}

	bool setSparseValues(int *keyBuffer, int *valueBuffer)
	{
		this->elementIdentifier = static_cast<DsLabelIdentifier>(*keyBuffer);
		this->elementIndex = this->mesh->findIndexByIdentifier(this->elementIdentifier);
		if (elementIndex == DS_LABEL_INDEX_INVALID)
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Could not find %d-D element %d used as key in sparse parameters for element local-to-global nodes map",
				this->mesh->getDimension(), this->elementIdentifier);
			return false;
		}
		return setDenseValues(valueBuffer);
	}

};

class DsLabelIdentifierArrayParameterConsumer
{
	const DsLabels &labels;
	DsLabelIdentifierArray &labelsIdentifierArray;
	bool isDense;
	std::vector<FmlObjectHandle> fmlDenseIndexEvaluators;
	std::vector<FmlObjectHandle> fmlSparseIndexEvaluators;
	int sparseRecordCount;
	int recordCount;
	int recordOffsets[2];
	int recordSizes[2];
	// for dense parameters only:
	DsLabelIndex labelIndex;
	DsLabelIdentifier labelIdentifier;

public:
	DsLabelIdentifierArrayParameterConsumer(const DsLabels &labelsIn, DsLabelIdentifierArray &labelsIdentifierArrayIn) :
		labels(labelsIn),
		labelsIdentifierArray(labelsIdentifierArrayIn),
		isDense(false),
		sparseRecordCount(0),
		recordCount(0),
		labelIndex(-1)
	{
		this->recordSizes[0] = 1;
		this->recordSizes[1] = 1;
		this->recordOffsets[0] = -1;
		this->recordOffsets[1] = 0;
	}

	/** Must be called before all other methods to set up indexing */
	bool setIndexing(std::vector<FmlObjectHandle>& fmlSparseIndexEvaluatorsIn, int sparseRecordCountIn, std::vector<FmlObjectHandle>& fmlDenseIndexEvaluatorsIn)
	{
		if ((fmlSparseIndexEvaluatorsIn.size() + fmlDenseIndexEvaluatorsIn.size()) != 1)
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Expected 1 index for label identifier map parameters");
			return false;
		}
		this->fmlSparseIndexEvaluators = fmlSparseIndexEvaluatorsIn;
		this->sparseRecordCount = sparseRecordCountIn;
		this->fmlDenseIndexEvaluators = fmlDenseIndexEvaluatorsIn;
		this->isDense = fmlSparseIndexEvaluatorsIn.size() == 0;
		this->recordCount = (this->isDense) ? this->labels.getSize() : this->sparseRecordCount;
		if (this->recordCount < 0)
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Invalid number of records for label identifier map parameters");
			return false;
		}
		return true;
	}

	/** Only call after setIndexing() */
	int getRecordCount() const
	{
		return this->recordCount;
	}

	bool nextRecord()
	{
		++this->recordOffsets[0];
		if (this->isDense)
		{
			++this->labelIndex;
			this->labelIdentifier = this->labels.getIdentifier(this->labelIndex);
			if (DS_LABEL_IDENTIFIER_INVALID == this->labelIdentifier)
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  DsLabelIdentifierArrayParameterConsumer::nextRecord() found no element for index");
				return false;
			}
		}
		return true;
	}

	int getDenseRecordBufferSize() const
	{
		return 1;
	}

	const int *getDenseRecordOffsets() const
	{
		return this->recordOffsets;
	}

	const int *getDenseRecordSizes() const
	{
		return this->recordSizes;
	}

	const int *getSparseRecordOffsets() const
	{
		return this->recordOffsets;
	}

	const int *getSparseRecordSizes() const
	{
		return this->recordSizes;
	}

	/** @param valueBuffer  Array of node identifiers, values of -1 mean no node set at index */
	bool setDenseValues(int *valueBuffer)
	{
		if (!this->labelsIdentifierArray.setValue(this->labelIndex, static_cast<DsLabelIdentifier>(*valueBuffer)))
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  DsLabelIdentifierArrayParameterConsumer failed to set identifier for index %d (identifier %d)",
				this->labelIndex, this->labelIdentifier);
			return false;
		}
		return true;
	}

	bool setSparseValues(int *keyBuffer, int *valueBuffer)
	{
		this->labelIdentifier = static_cast<DsLabelIdentifier>(*keyBuffer);
		this->labelIndex = this->labels.findLabelByIdentifier(this->labelIdentifier);
		if (DS_LABEL_INDEX_INVALID == this->labelIndex)
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Could not find label with identifier %d used as key in sparse parameters for label identifier map parameters",
				this->labelIdentifier);
			return false;
		}
		return setDenseValues(valueBuffer);
	}

};

/**
 * Gets handle to DsLabels matching fmlEnsembleType. Definition is read from
 * FieldML document only when first requested.
 * ???GRC Assumes there is only one argument for each type; consider having a
 * separate class to represent each argument.
 *
 * @param fmlEnsembleType  Handle of type FHT_ENSEMBLE_TYPE.
 * @return  Accessed pointer to labels, or 0 on failure */
DsLabels *FieldMLReader::getLabelsForEnsemble(FmlObjectHandle fmlEnsembleType)
{
	FmlObjectLabelsMap::iterator iterator = this->labelsMap.find(fmlEnsembleType);
	if (iterator != labelsMap.end())
		return cmzn::Access(cmzn::GetImpl(iterator->second));

	std::string name = this->getName(fmlEnsembleType);
	if (name.length()==0)
	{
		// GRC workaround for ensemble types that have not been imported
		name = "NONIMPORTED_";
		name.append(getDeclaredName(fmlEnsembleType));
	}
	if (Fieldml_GetObjectType(fmlSession, fmlEnsembleType) != FHT_ENSEMBLE_TYPE)
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::getLabelsForEnsemble:  Argument %s is not ensemble type", name.c_str());
		return 0;
	}

	FieldmlEnsembleMembersType fmlEnsembleMembersType = Fieldml_GetEnsembleMembersType(fmlSession, fmlEnsembleType);
	int recordSize = 0;
	switch (fmlEnsembleMembersType)
	{
	case FML_ENSEMBLE_MEMBER_RANGE:
		break;
	case FML_ENSEMBLE_MEMBER_LIST_DATA:
		recordSize = 1;
		break;
	case FML_ENSEMBLE_MEMBER_RANGE_DATA:
		recordSize = 2;
		break;
	case FML_ENSEMBLE_MEMBER_STRIDE_RANGE_DATA:
		recordSize = 3;
		break;
	case FML_ENSEMBLE_MEMBER_UNKNOWN:
	default:
		display_message(ERROR_MESSAGE, "Read FieldML:  Unsupported members type %d for ensemble type %s",
			fmlEnsembleMembersType, name.c_str());
		return 0;
		break;
	}
	if (verbose)
	{
		display_message(INFORMATION_MESSAGE, "Reading ensemble type %s\n", name.c_str());
	}
	HDsLabels labels(new DsLabels());
	labels->setName(name);
	this->setProcessed(fmlEnsembleType);
	this->labelsMap[fmlEnsembleType] = labels;

	int return_code = 1;
	if (FML_ENSEMBLE_MEMBER_RANGE == fmlEnsembleMembersType)
	{
		FmlEnsembleValue min = Fieldml_GetEnsembleMembersMin(fmlSession, fmlEnsembleType);
		FmlEnsembleValue max = Fieldml_GetEnsembleMembersMax(fmlSession, fmlEnsembleType);
		int stride = Fieldml_GetEnsembleMembersStride(fmlSession, fmlEnsembleType);
		int result = labels->addLabelsRange(min, max, stride);
		if (result != CMZN_OK)
			return 0;
	}
	else
	{
		const int memberCount = Fieldml_GetMemberCount(fmlSession, fmlEnsembleType);
		FmlObjectHandle fmlDataSource = Fieldml_GetDataSource(fmlSession, fmlEnsembleType);
		int arrayRank = 0;
		int arraySizes[2];
		if (fmlDataSource == FML_INVALID_HANDLE)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Could not get data source for ensemble type %s", name.c_str());
			return_code = 0;
		}
		else if (FML_DATA_SOURCE_ARRAY != Fieldml_GetDataSourceType(fmlSession, fmlDataSource))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Can only define ensemble types from array data source; processing %s", name.c_str());
			return_code = 0;
		}
		else if (2 != (arrayRank = Fieldml_GetArrayDataSourceRank(fmlSession, fmlDataSource)))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Expected array data source of rank 2; processing %s", name.c_str());
			return_code = 0;
		}
		else if ((FML_ERR_NO_ERROR != Fieldml_GetArrayDataSourceSizes(fmlSession, fmlDataSource, arraySizes)) ||
			(arraySizes[0] < 1) || (arraySizes[1] != recordSize))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Invalid data source sizes; processing %s", name.c_str());
			return_code = 0;
		}
		else
		{
			FmlReaderHandle fmlReader = Fieldml_OpenReader(fmlSession, fmlDataSource);
			int *rangeData = new int[arraySizes[0]*arraySizes[1]];
			const int arrayOffsets[2] = { 0, 0 };
			FmlIoErrorNumber ioResult = FML_IOERR_NO_ERROR;
			if (fmlReader == FML_INVALID_HANDLE)
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Could not open reader for ensemble type %s", name.c_str());
				return_code = 0;
			}
			else if (FML_IOERR_NO_ERROR !=
				(ioResult = Fieldml_ReadIntSlab(fmlReader, arrayOffsets, arraySizes, rangeData)))
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Error reading array data source %s", getName(fmlDataSource).c_str());
				return_code = 0;
			}
			else
			{
				const int recordCount = arraySizes[0];
				for (int i = 0; i < recordCount; i++)
				{
					switch (fmlEnsembleMembersType)
					{
					case FML_ENSEMBLE_MEMBER_LIST_DATA:
					{
						int result = labels->findOrCreateLabel(rangeData[i]);
						if (result != CMZN_OK)
							return_code = 0;
					} break;
					case FML_ENSEMBLE_MEMBER_RANGE_DATA:
					{
						int result = labels->addLabelsRange(/*min*/rangeData[i*2], /*max*/rangeData[i*2 + 1]);
						if (result != CMZN_OK)
							return_code = 0;
					} break;
					case FML_ENSEMBLE_MEMBER_STRIDE_RANGE_DATA:
					{
						int result = labels->addLabelsRange(/*min*/rangeData[i*3], /*max*/rangeData[i*3 + 1], /*stride*/rangeData[i*3 + 2]);
						if (result != CMZN_OK)
							return_code = 0;
					} break;
					default:
						// should never happen - see switch above
						display_message(ERROR_MESSAGE, "Read FieldML:  Unexpected ensemble members type");
						return_code = 0;
						break;
					}
					if (!return_code)
					{
						break;
					}
				}
				if (return_code && (labels->getSize() != memberCount))
				{
					display_message(ERROR_MESSAGE, "Read FieldML:  Ensemble type %s lists member count %d, actual number in data source is %d",
						name.c_str(), memberCount, labels->getSize());
					return_code = 0;
				}
			}
			delete[] rangeData;
			Fieldml_CloseReader(fmlReader);
		}
	}
	if (!return_code)
		return 0;
	return cmzn::Access(cmzn::GetImpl(labels));
}

// template and full specialisations to read different types with template
template <typename VALUETYPE> FmlIoErrorNumber FieldML_ReadSlab(
	FmlReaderHandle readerHandle, const int *offsets, const int *sizes, VALUETYPE *valueBuffer);

template <> inline FmlIoErrorNumber FieldML_ReadSlab(
	FmlReaderHandle readerHandle, const int *offsets, const int *sizes, double *valueBuffer)
{
	return Fieldml_ReadDoubleSlab(readerHandle, offsets, sizes, valueBuffer);
}

template <> inline FmlIoErrorNumber FieldML_ReadSlab(
	FmlReaderHandle readerHandle, const int *offsets, const int *sizes, int *valueBuffer)
{
	return Fieldml_ReadIntSlab(readerHandle, offsets, sizes, valueBuffer);
}

// TODO : Support order
// ???GRC can order cover subset of ensemble?
template <typename VALUETYPE, class PARAMETERCONSUMER> bool FieldMLReader::readParameters(FmlObjectHandle fmlParameters,
	PARAMETERCONSUMER& parameterConsumer)
{
	std::string name = this->getName(fmlParameters);
	FieldmlDataDescriptionType dataDescription = Fieldml_GetParameterDataDescription(fmlSession, fmlParameters);
	if ((dataDescription != FML_DATA_DESCRIPTION_DENSE_ARRAY) &&
		(dataDescription != FML_DATA_DESCRIPTION_DOK_ARRAY))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Unknown data description for parameters %s; must be dense array or DOK array", name.c_str());
		return false;
	}

	FmlObjectHandle fmlDataSource = Fieldml_GetDataSource(fmlSession, fmlParameters);
	if (fmlDataSource == FML_INVALID_HANDLE)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Could not get data source for parameters %s", name.c_str());
		return false;
	}
	if (FML_DATA_SOURCE_ARRAY != Fieldml_GetDataSourceType(fmlSession, fmlDataSource))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Only supports ArrayDataSource for parameters %s", name.c_str());
		return false;
	}
	const int denseIndexCount = Fieldml_GetParameterIndexCount(fmlSession, fmlParameters, /*isSparse*/0);
	const int expectedArrayRank = (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY) ? 2 : denseIndexCount;
	const int arrayRank = Fieldml_GetArrayDataSourceRank(fmlSession, fmlDataSource);
	if (arrayRank != expectedArrayRank)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Data source %s has invalid rank for parameters %s",
			getName(fmlDataSource).c_str(), name.c_str());
		return false;
	}

	std::vector<int> arrayRawSizes(expectedArrayRank, 0);
	std::vector<int> arrayOffsets(expectedArrayRank, 0);
	std::vector<int> arraySizes(expectedArrayRank, 0);
	if ((arrayRank > 0) && (
		(FML_ERR_NO_ERROR != Fieldml_GetArrayDataSourceRawSizes(fmlSession, fmlDataSource, arrayRawSizes.data())) ||
		(FML_ERR_NO_ERROR != Fieldml_GetArrayDataSourceOffsets(fmlSession, fmlDataSource, arrayOffsets.data())) ||
		(FML_ERR_NO_ERROR != Fieldml_GetArrayDataSourceSizes(fmlSession, fmlDataSource, arraySizes.data()))))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Failed to get array sizes of data source %s for parameters %s",
			getName(fmlDataSource).c_str(), name.c_str());
		return false;
	}
	// array size of 0 means 'all of raw size after offset', so calculate effective size.
	for (int r = 0; r < arrayRank; r++)
	{
		if (arraySizes[r] == 0)
			arraySizes[r] = arrayRawSizes[r] - arrayOffsets[r];
	}

	std::vector<FmlObjectHandle> fmlDenseIndexEvaluators(denseIndexCount, FML_INVALID_OBJECT_HANDLE);
	for (int i = 0; i < denseIndexCount; ++i)
	{
		fmlDenseIndexEvaluators[i] = Fieldml_GetParameterIndexEvaluator(this->fmlSession, fmlParameters, i + 1, /*isSparse*/0);
		FmlObjectHandle fmlDenseIndexType = Fieldml_GetValueType(fmlSession, fmlDenseIndexEvaluators[i]);
		if (dataDescription == FML_DATA_DESCRIPTION_DENSE_ARRAY)
		{
			const int memberCount = Fieldml_GetMemberCount(fmlSession, fmlDenseIndexType);
			if (memberCount != arraySizes[i])
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Data source %s size[%d]=%d differs from size of dense index %s (%d) for dense parameters %s",
					getName(fmlDataSource).c_str(), i + 1, arraySizes[i],
					getName(fmlDenseIndexEvaluators[i]).c_str(), memberCount, name.c_str());
				return false;
			}
		}
		FmlObjectHandle fmlOrderDataSource = Fieldml_GetParameterIndexOrder(fmlSession, fmlParameters, i + 1);
		if (fmlOrderDataSource != FML_INVALID_HANDLE)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Parameters %s dense index %s specifies order. This is not yet implemented.",
				name.c_str(), getName(fmlDenseIndexEvaluators[i]).c_str());
			return false;
		}
	}
	if (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY)
	{
		// always rank 2; second size is product of all dense indexes
		int denseSize = 1;
		for (int i = 0; i < denseIndexCount; ++i)
			denseSize *= Fieldml_GetMemberCount(fmlSession, Fieldml_GetValueType(fmlSession, fmlDenseIndexEvaluators[i]));
		if (denseSize != arraySizes[1])
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Data source %s size[2]=%d differs from product of member count of dense indexes (%d) for sparse parameters %s",
				getName(fmlDataSource).c_str(), arraySizes[1], denseSize, name.c_str());
			return false;
		}
	}

	const int sparseIndexCount = (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY) ?
		Fieldml_GetParameterIndexCount(fmlSession, fmlParameters, /*isSparse*/1) : 0;
	std::vector<FmlObjectHandle> fmlSparseIndexEvaluators(sparseIndexCount, FML_INVALID_OBJECT_HANDLE);
	FmlObjectHandle fmlKeyDataSource = FML_INVALID_HANDLE;
	int keyArrayRawSizes[2];
	int keyArraySizes[2];
	int keyArrayOffsets[2];
	int sparseRecordCount = 0;
	if (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY)
	{
		fmlKeyDataSource = Fieldml_GetKeyDataSource(fmlSession, fmlParameters);
		if (fmlKeyDataSource == FML_INVALID_HANDLE)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Could not get key data source for parameters %s", name.c_str());
			return false;
		}
		if (FML_DATA_SOURCE_ARRAY != Fieldml_GetDataSourceType(fmlSession, fmlKeyDataSource))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Only supports ArrayDataSource for keys for parameters %s", name.c_str());
			return false;
		}
		if (Fieldml_GetArrayDataSourceRank(fmlSession, fmlKeyDataSource) != 2)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Key data source %s for parameters %s must be rank 2",
				getName(fmlKeyDataSource).c_str(), name.c_str());
			return false;
		}
		if ((FML_ERR_NO_ERROR != Fieldml_GetArrayDataSourceRawSizes(fmlSession, fmlKeyDataSource, keyArrayRawSizes)) ||
			(FML_ERR_NO_ERROR != Fieldml_GetArrayDataSourceSizes(fmlSession, fmlKeyDataSource, keyArraySizes)) ||
			(FML_ERR_NO_ERROR != Fieldml_GetArrayDataSourceOffsets(fmlSession, fmlKeyDataSource, keyArrayOffsets)))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Failed to get array sizes for key data source %s for parameters %s",
				getName(fmlKeyDataSource).c_str(), name.c_str());
			return false;
		}
		// zero array size means use raw size less offset
		for (int r = 0; r < 2; ++r)
		{
			if (keyArraySizes[r] == 0)
				keyArraySizes[r] = keyArrayRawSizes[r] - keyArrayOffsets[r];
		}
		if ((keyArraySizes[0] != arraySizes[0]) || (keyArraySizes[1] != sparseIndexCount))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Invalid array sizes for key data source %s for parameters %s",
				getName(fmlKeyDataSource).c_str(), name.c_str());
			return false;
		}
		for (int i = 0; i < sparseIndexCount; i++)
		{
			fmlSparseIndexEvaluators[i] = Fieldml_GetParameterIndexEvaluator(this->fmlSession, fmlParameters, i + 1, /*isSparse*/1);
		}
		sparseRecordCount = arraySizes[0];
	}

	if (!parameterConsumer.setIndexing(fmlSparseIndexEvaluators, sparseRecordCount, fmlDenseIndexEvaluators))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Failed to set index evaluators for consumer of parameters %s.", name.c_str());
		return false;
	}

	std::vector<int> keyVector(sparseIndexCount);
	int *keyBuffer = keyVector.data();
	const int valueBufferSize = parameterConsumer.getDenseRecordBufferSize();
	std::vector<VALUETYPE> valueVector(valueBufferSize);
	VALUETYPE *valueBuffer = valueVector.data();

	FmlReaderHandle fmlValueReader = Fieldml_OpenReader(fmlSession, fmlDataSource);
	if (fmlValueReader == FML_INVALID_HANDLE)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Could not open reader for parameters %s data source %s",
			name.c_str(), getName(fmlDataSource).c_str());
		return false;
	}
	FmlReaderHandle fmlKeyReader = FML_INVALID_HANDLE;
	if (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY)
	{
		fmlKeyReader = Fieldml_OpenReader(fmlSession, fmlKeyDataSource);
		if (fmlKeyReader == FML_INVALID_HANDLE)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Could not open reader for parameters %s key data source %s",
				name.c_str(), getName(fmlKeyDataSource).c_str());
			Fieldml_CloseReader(fmlValueReader);
			return false;
		}
	}

	bool result = true;
	const int recordCount = parameterConsumer.getRecordCount();
	const int *denseRecordSizes = parameterConsumer.getDenseRecordSizes();
	FmlIoErrorNumber ioResult;
	if (dataDescription == FML_DATA_DESCRIPTION_DENSE_ARRAY)
	{
		for (int r = 0; r < recordCount; ++r)
		{
			if (!parameterConsumer.nextRecord())
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  Unexpected end of records when reading dense parameter evaluator %s", name.c_str());
				result = false;
				break;
			}
			ioResult = FieldML_ReadSlab(fmlValueReader, parameterConsumer.getDenseRecordOffsets(), denseRecordSizes, valueBuffer);
			if (ioResult != FML_IOERR_NO_ERROR)
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  Failed to read values data source %s for dense parameters %s",
					getName(fmlDataSource).c_str(), name.c_str());
				result = false;
				break;
			}
			if (!parameterConsumer.setDenseValues(valueBuffer))
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  Failed to set dense values read from data source %s for parameters %s",
					getName(fmlDataSource).c_str(), name.c_str());
				result = false;
				break;
			}
		}
	}
	else
	{
		const int *sparseRecordSizes = parameterConsumer.getSparseRecordSizes();
		for (int r = 0; r < recordCount; ++r)
		{
			if (!parameterConsumer.nextRecord())
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  Unexpected end of records when reading sparse parameter evaluator %s", name.c_str());
				result = false;
				break;
			}
			ioResult = Fieldml_ReadIntSlab(fmlKeyReader, parameterConsumer.getSparseRecordOffsets(), sparseRecordSizes, keyBuffer);
			if (ioResult != FML_IOERR_NO_ERROR)
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  Failed to read key data source %s for parameters %s",
					getName(fmlKeyDataSource).c_str(), name.c_str());
				result = false;
				break;
			}
			ioResult = FieldML_ReadSlab(fmlValueReader, parameterConsumer.getDenseRecordOffsets(), denseRecordSizes, valueBuffer);
			if (ioResult != FML_IOERR_NO_ERROR)
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  Failed to read values data source %s for sparse parameters %s",
					getName(fmlDataSource).c_str(), name.c_str());
				result = false;
				break;
			}
			if (!parameterConsumer.setSparseValues(keyBuffer, valueBuffer))
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  Failed to set sparse values read from data source %s for parameters %s",
					getName(fmlDataSource).c_str(), name.c_str());
				result = false;
				break;
			}
		}
	}

	if (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY)
		Fieldml_CloseReader(fmlKeyReader);
	Fieldml_CloseReader(fmlValueReader);
	return result;
}

/**
 * Returns integer map for supplied parameters, reading it from the data source
 * if encountered for the first time.
 *
 * @param fmlParameters  Handle of type FHT_PARAMETER_EVALUATOR.
 * @return  Accessed pointer to integer map, or 0 on failure */
DsMap<int> *FieldMLReader::getEnsembleParameters(FmlObjectHandle fmlParameters)
{
	FmlObjectIntParametersMap::iterator iter = this->intParametersMap.find(fmlParameters);
	if (iter != this->intParametersMap.end())
		return cmzn::Access(cmzn::GetImpl(iter->second));

	std::string name = getName(fmlParameters);
	if (Fieldml_GetObjectType(this->fmlSession, fmlParameters) != FHT_PARAMETER_EVALUATOR)
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::getEnsembleParameters.  %s is not a parameter evaluator", name.c_str());
		return 0;
	}
	FmlObjectHandle fmlValueType = Fieldml_GetValueType(fmlSession, fmlParameters);
	FieldmlHandleType valueClass = Fieldml_GetObjectType(fmlSession, fmlValueType);
	if (valueClass != FHT_ENSEMBLE_TYPE)
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::getEnsembleParameters.  %s is not ensemble-valued", name.c_str());
		return 0;
	}

	if (verbose)
		display_message(INFORMATION_MESSAGE, "Reading ensemble parameters %s\n", name.c_str());
	int indexCount = Fieldml_GetIndexEvaluatorCount(this->fmlSession, fmlParameters);
	std::vector<HDsLabels> indexingLabelsVector;
	int return_code = 1;
	for (int indexNumber = 1; indexNumber <= indexCount; ++indexNumber)
	{
		FmlObjectHandle fmlIndexEvaluator = Fieldml_GetIndexEvaluator(this->fmlSession, fmlParameters, indexNumber);
		FmlObjectHandle fmlEnsembleType = Fieldml_GetValueType(this->fmlSession, fmlIndexEvaluator);
		if ((FHT_ARGUMENT_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlIndexEvaluator)) ||
			(FHT_ENSEMBLE_TYPE != Fieldml_GetObjectType(this->fmlSession, fmlEnsembleType)))
		{
			display_message(WARNING_MESSAGE, "FieldMLReader::getEnsembleParameters:  Index %d (%s) of parameters %s is not an ensemble-valued argument evaluator",
				indexNumber, getName(fmlIndexEvaluator).c_str(), name.c_str());
			return_code = 0;
		}
		if (verbose)
			display_message(INFORMATION_MESSAGE, "  Index %d = %s\n", indexNumber, getName(fmlIndexEvaluator).c_str());
		indexingLabelsVector.push_back(HDsLabels(this->getLabelsForEnsemble(fmlEnsembleType)));
	}
	if (!return_code)
		return 0;
	HDsMapInt parameters(DsMap<int>::create(indexingLabelsVector));
	parameters->setName(name);
	DsMapParameterConsumer<int> mapConsumer(*this, *parameters);
	if (!this->readParameters<int>(fmlParameters, mapConsumer))
		return 0;
	this->setProcessed(fmlParameters);
	this->intParametersMap[fmlParameters] = parameters;
	return cmzn::Access(cmzn::GetImpl(parameters));
}

/**
 * Returns double map for supplied parameters, reading it from the data source
 * if encountered for the first time.
 *
 * @param fmlParameters  Handle of type FHT_PARAMETER_EVALUATOR.
 * @return  Accessed pointer to double map, or 0 on failure */
DsMap<double> *FieldMLReader::getContinuousParameters(FmlObjectHandle fmlParameters)
{
	FmlObjectDoubleParametersMap::iterator iter = this->doubleParametersMap.find(fmlParameters);
	if (iter != this->doubleParametersMap.end())
		return cmzn::Access(cmzn::GetImpl(iter->second));

	std::string name = getName(fmlParameters);
	if (Fieldml_GetObjectType(this->fmlSession, fmlParameters) != FHT_PARAMETER_EVALUATOR)
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::getContinuousParameters.  %s is not a parameter evaluator", name.c_str());
		return 0;
	}
	FmlObjectHandle fmlValueType = Fieldml_GetValueType(fmlSession, fmlParameters);
	FieldmlHandleType valueClass = Fieldml_GetObjectType(fmlSession, fmlValueType);
	if (valueClass != FHT_CONTINUOUS_TYPE)
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::getContinuousParameters.  %s is not continuous-valued", name.c_str());
		return 0;
	}
	FmlObjectHandle fmlComponentsType = Fieldml_GetTypeComponentEnsemble(fmlSession, fmlValueType);
	if (fmlComponentsType != FML_INVALID_OBJECT_HANDLE)
	{
		display_message(WARNING_MESSAGE, "FieldMLReader::getContinuousParameters:  Cannot read non-scalar parameters %s", name.c_str());
		return 0;
	}

	if (verbose)
		display_message(INFORMATION_MESSAGE, "Reading continuous parameters %s\n", name.c_str());
	int indexCount = Fieldml_GetIndexEvaluatorCount(this->fmlSession, fmlParameters);
	std::vector<HDsLabels> indexingLabelsVector;
	int return_code = 1;
	for (int indexNumber = 1; indexNumber <= indexCount; ++indexNumber)
	{
		FmlObjectHandle fmlIndexEvaluator = Fieldml_GetIndexEvaluator(this->fmlSession, fmlParameters, indexNumber);
		FmlObjectHandle fmlEnsembleType = Fieldml_GetValueType(this->fmlSession, fmlIndexEvaluator);
		if ((FHT_ARGUMENT_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlIndexEvaluator)) ||
			(FHT_ENSEMBLE_TYPE != Fieldml_GetObjectType(this->fmlSession, fmlEnsembleType)))
		{
			display_message(WARNING_MESSAGE, "FieldMLReader::getContinuousParameters:  Index %d (%s) of parameters %s is not an ensemble-valued argument evaluator",
				indexNumber, getName(fmlIndexEvaluator).c_str(), name.c_str());
			return_code = 0;
		}
		if (verbose)
			display_message(INFORMATION_MESSAGE, "  Index %d = %s\n", indexNumber, getName(fmlIndexEvaluator).c_str());
		indexingLabelsVector.push_back(HDsLabels(this->getLabelsForEnsemble(fmlEnsembleType)));
	}
	if (!return_code)
		return 0;
	HDsMapDouble parameters(DsMap<double>::create(indexingLabelsVector));
	parameters->setName(name);
	DsMapParameterConsumer<double> mapConsumer(*this, *parameters);
	if (!this->readParameters<double>(fmlParameters, mapConsumer))
		return 0;
	this->setProcessed(fmlParameters);
	this->doubleParametersMap[fmlParameters] = parameters;
	return cmzn::Access(cmzn::GetImpl(parameters));
}

int FieldMLReader::readNodes(FmlObjectHandle fmlNodesArgumentIn)
{
	if (FML_INVALID_OBJECT_HANDLE != this->fmlNodesType)
		return CMZN_ERROR_ALREADY_EXISTS;
	FmlObjectHandle fmlNodesTypeIn = Fieldml_GetValueType(this->fmlSession, fmlNodesArgumentIn);
	HDsLabels nodesLabels(this->getLabelsForEnsemble(fmlNodesTypeIn));
	if (!nodesLabels)
		return CMZN_ERROR_GENERAL;
	int return_code = CMZN_OK;
	cmzn_nodeset_id nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(this->field_module, CMZN_FIELD_DOMAIN_TYPE_NODES);
	cmzn_nodetemplate_id nodetemplate = cmzn_nodeset_create_nodetemplate(nodeset);
	DsLabelIterator *nodesLabelIterator = nodesLabels->createLabelIterator();
	if (!nodesLabelIterator)
		return_code = CMZN_ERROR_MEMORY;
	else
	{
		while (nodesLabelIterator->increment())
		{
			DsLabelIdentifier nodeIdentifier = nodesLabelIterator->getIdentifier();
			cmzn_node_id node = cmzn_nodeset_create_node(nodeset, nodeIdentifier, nodetemplate);
			if (!node)
			{
				return_code = CMZN_ERROR_MEMORY;
				break;
			}
			cmzn_node_destroy(&node);
		}
	}
	cmzn::Deaccess(nodesLabelIterator);
	cmzn_nodetemplate_destroy(&nodetemplate);
	cmzn_nodeset_destroy(&nodeset);
	if (CMZN_OK == return_code)
	{
		this->fmlNodesType = fmlNodesTypeIn;
		this->fmlNodesArgument = fmlNodesArgumentIn;
	}
	return return_code;
}

int FieldMLReader::readGlobals()
{
	// if nodes not found, use legacy behaviour i.e. nodes are free parameter index
	std::string nodesTypeName("nodes");
	std::string nodesArgumentName = nodesTypeName + ".argument";
	FmlObjectHandle fmlPossibleNodesType = Fieldml_GetObjectByName(this->fmlSession, nodesTypeName.c_str());
	FmlObjectHandle fmlPossibleNodesArgument = Fieldml_GetObjectByName(this->fmlSession, nodesArgumentName.c_str());
	if ((FML_INVALID_OBJECT_HANDLE != fmlPossibleNodesType) &&
		(FML_INVALID_OBJECT_HANDLE != fmlPossibleNodesArgument))
	{
		if ((FHT_ENSEMBLE_TYPE != Fieldml_GetObjectType(this->fmlSession, fmlPossibleNodesType)) ||
			(FHT_ARGUMENT_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlPossibleNodesArgument)) ||
			(Fieldml_GetValueType(this->fmlSession, fmlPossibleNodesArgument) != fmlPossibleNodesType))
		{
			display_message(WARNING_MESSAGE, "Read FieldML:  Non-standard definition of %s or %s. Ignoring.",
				nodesTypeName.c_str(), nodesArgumentName.c_str());
		}
		else
		{
			int return_code = this->readNodes(fmlPossibleNodesArgument);
			if (CMZN_OK != return_code)
				return 0;
		}
	}
	// if node_derivatives not found, use legacy behaviour i.e. no node derivatives
	std::string nodeDerivativesTypeName("node_derivatives");
	std::string nodeDerivativesArgumentName = nodeDerivativesTypeName + ".argument";
	this->fmlNodeDerivativesType = Fieldml_GetObjectByName(this->fmlSession, nodeDerivativesTypeName.c_str());
	this->fmlNodeDerivativesArgument = Fieldml_GetObjectByName(this->fmlSession, nodeDerivativesArgumentName.c_str());
	if ((FML_INVALID_OBJECT_HANDLE != this->fmlNodeDerivativesType) ||
		(FML_INVALID_OBJECT_HANDLE != this->fmlNodeDerivativesArgument))
	{
		if ((FHT_ENSEMBLE_TYPE != Fieldml_GetObjectType(this->fmlSession, this->fmlNodeDerivativesType)) ||
			(FHT_ARGUMENT_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, this->fmlNodeDerivativesArgument)) ||
			(Fieldml_GetValueType(this->fmlSession, this->fmlNodeDerivativesArgument) != this->fmlNodeDerivativesType))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Non-standard definition of %s or %s.",
				nodeDerivativesTypeName.c_str(), nodeDerivativesArgumentName.c_str());
			return 0;
		}
		HDsLabels tmpNodeDerivatives(this->getLabelsForEnsemble(this->fmlNodeDerivativesType));
		if (!tmpNodeDerivatives)
			return 0;
	}
	// if node_versions not found, use legacy behaviour i.e. no node versions
	std::string nodeVersionsTypeName("node_versions");
	std::string nodeVersionsArgumentName = nodeVersionsTypeName + ".argument";
	this->fmlNodeVersionsType = Fieldml_GetObjectByName(this->fmlSession, nodeVersionsTypeName.c_str());
	this->fmlNodeVersionsArgument = Fieldml_GetObjectByName(this->fmlSession, nodeVersionsArgumentName.c_str());
	if ((FML_INVALID_OBJECT_HANDLE != this->fmlNodeVersionsType) ||
		(FML_INVALID_OBJECT_HANDLE != this->fmlNodeVersionsArgument))
	{
		if ((FHT_ENSEMBLE_TYPE != Fieldml_GetObjectType(this->fmlSession, this->fmlNodeVersionsType)) ||
			(FHT_ARGUMENT_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, this->fmlNodeVersionsArgument)) ||
			(Fieldml_GetValueType(this->fmlSession, this->fmlNodeVersionsArgument) != this->fmlNodeVersionsType))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Non-standard definition of %s or %s.",
				nodeVersionsTypeName.c_str(), nodeVersionsArgumentName.c_str());
			return 0;
		}
		HDsLabels tmpNodeVersions(this->getLabelsForEnsemble(this->fmlNodeVersionsType));
		if (!tmpNodeVersions)
			return 0;
	}
	return 1;
}

FmlObjectHandle FieldMLReader::getArgumentForType(FmlObjectHandle fmlValueType)
{
	FmlObjectHandle fmlArgument = FML_INVALID_OBJECT_HANDLE;
	const int argumentCount = Fieldml_GetObjectCount(this->fmlSession, FHT_ARGUMENT_EVALUATOR);
	for (int i = 1; i <= argumentCount; ++i)
	{
		FmlObjectHandle fmlTempArgument = Fieldml_GetObject(this->fmlSession, FHT_ARGUMENT_EVALUATOR, i);
		if (Fieldml_GetValueType(this->fmlSession, fmlTempArgument) == fmlValueType)
		{
			if (fmlArgument != FML_INVALID_OBJECT_HANDLE)
			{
				display_message(ERROR_MESSAGE, "FieldML Reader.  Multiple arguments found for type %s, which is not supported.",
					this->getName(fmlValueType).c_str());
				return FML_INVALID_OBJECT_HANDLE;
			}
			fmlArgument = fmlTempArgument;
		}
	}
	return fmlArgument;
}

int FieldMLReader::readMeshes()
{
	const int meshCount = Fieldml_GetObjectCount(this->fmlSession, FHT_MESH_TYPE);
	if (meshCount != 1)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Limited to 1 mesh type, %d found", meshCount);
		return 0;
	}
	int return_code = 1;
	for (int meshIndex = 1; (meshIndex <= meshCount) && return_code; meshIndex++)
	{
		this->fmlMeshType = Fieldml_GetObject(this->fmlSession, FHT_MESH_TYPE, meshIndex);
		std::string name = getName(fmlMeshType);
		// get assumed sole argument for this mesh
		this->fmlMeshArgument = this->getArgumentForType(this->fmlMeshType);
		FmlObjectHandle fmlMeshChartType = Fieldml_GetMeshChartType(fmlSession, fmlMeshType);
		this->fmlMeshChartArgument = this->getArgumentForType(fmlMeshChartType);
		this->fmlElementsType = Fieldml_GetMeshElementsType(fmlSession, fmlMeshType);
		this->fmlElementsArgument = this->getArgumentForType(fmlElementsType);
		if ((this->fmlMeshArgument == FML_INVALID_OBJECT_HANDLE)
			|| (this->fmlMeshChartArgument == FML_INVALID_OBJECT_HANDLE)
			|| (this->fmlElementsArgument == FML_INVALID_OBJECT_HANDLE))
		{
			display_message(ERROR_MESSAGE, "FieldML Reader.  Could not get exactly one mesh/chart/elements argument for mesh %s.",
				this->getName(this->fmlMeshType).c_str());
			return_code = 0;
			break;
		}
		this->fmlNodeParametersArgument = FML_INVALID_OBJECT_HANDLE; // discovered on-the-fly
		FmlObjectHandle fmlMeshChartComponentType = Fieldml_GetTypeComponentEnsemble(fmlSession, fmlMeshChartType);
		int meshDimension = Fieldml_GetMemberCount(fmlSession, fmlMeshChartComponentType);
		if ((meshDimension < 1) || (meshDimension > 3))
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Invalid dimension %d for mesh type %s", meshDimension, name.c_str());
			return_code = 0;
			break;
		}
		FE_region *fe_region = this->region->get_FE_region();
		this->mesh = FE_region_find_FE_mesh_by_dimension(fe_region, meshDimension);
		if (!this->mesh)
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Failed to get mesh of dimension %d for mesh type %s", meshDimension, name.c_str());
			return_code = 0;
			break;
		}
		if (verbose)
		{
			display_message(INFORMATION_MESSAGE, "Reading mesh '%s' dimension %d\n", name.c_str(), meshDimension);
		}

		HDsLabels elementsLabels(getLabelsForEnsemble(fmlElementsType));
		if (verbose)
		{
			display_message(INFORMATION_MESSAGE, "Defining %d elements from %s\n",
				elementsLabels->getSize(), getName(fmlElementsType).c_str());
		}

		// determine element shape mapping

		FmlObjectHandle fmlShapeEvaluator = Fieldml_GetMeshShapes(fmlSession, fmlMeshType);
		cmzn_element_shape_type const_shape_type = CMZN_ELEMENT_SHAPE_TYPE_INVALID;
		HDsMapInt elementShapeParameters; // used only if shape evaluator uses indirect map
		if (fmlShapeEvaluator == FML_INVALID_OBJECT_HANDLE)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Missing shape evaluator for mesh type %s", name.c_str());
			return_code = 0;
		}
		else if (FHT_BOOLEAN_TYPE != Fieldml_GetObjectType(fmlSession, Fieldml_GetValueType(fmlSession, fmlShapeEvaluator)))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Non-boolean-valued shape evaluator for mesh type %s", name.c_str());
			return_code = 0;
		}
		else
		{
			// Note: external evaluator arguments are assumed to be 'used'
			int argumentCount = Fieldml_GetArgumentCount(fmlSession, fmlShapeEvaluator, /*isBound*/0, /*isUsed*/1);
			FmlObjectHandle fmlChartArgument = FML_INVALID_OBJECT_HANDLE;
			FmlObjectHandle fmlChartArgumentValue = FML_INVALID_OBJECT_HANDLE;
			FmlObjectHandle fmlElementsArgument = FML_INVALID_OBJECT_HANDLE;
			FmlObjectHandle fmlElementsArgumentValue = FML_INVALID_OBJECT_HANDLE;
			if (argumentCount > 0)
			{
				fmlChartArgument = Fieldml_GetArgument(fmlSession, fmlShapeEvaluator, 1, /*isBound*/0, /*isUsed*/1);
				fmlChartArgumentValue = Fieldml_GetValueType(fmlSession, fmlChartArgument);
			}
			if (argumentCount == 2)
			{
				fmlElementsArgument = Fieldml_GetArgument(fmlSession, fmlShapeEvaluator, 2, /*isBound*/0, /*isUsed*/1);
				fmlElementsArgumentValue = Fieldml_GetValueType(fmlSession, fmlElementsArgument);
				if (fmlElementsArgumentValue != fmlElementsType)
				{
					FmlObjectHandle tmp = fmlElementsArgument;
					fmlElementsArgument = fmlChartArgument;
					fmlChartArgument = tmp;
					tmp = fmlElementsArgumentValue;
					fmlElementsArgumentValue = fmlChartArgumentValue;
					fmlChartArgumentValue = tmp;
				}
			}
			FieldmlHandleType shapeEvaluatorValueType = Fieldml_GetObjectType(fmlSession, Fieldml_GetValueType(fmlSession, fmlShapeEvaluator));
			FieldmlHandleType chartArgumentValueType = (argumentCount > 0) ? Fieldml_GetObjectType(fmlSession, fmlChartArgumentValue) : FHT_UNKNOWN;
			if (((argumentCount != 1) && (argumentCount != 2)) ||
				(FHT_BOOLEAN_TYPE != shapeEvaluatorValueType) ||
				(FHT_CONTINUOUS_TYPE != chartArgumentValueType) ||
				(Fieldml_GetTypeComponentCount(fmlSession, fmlChartArgumentValue) != meshDimension) ||
				((argumentCount == 2) && (fmlElementsArgumentValue != fmlElementsType)))
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Shape evaluator %s for mesh type %s must be a boolean evaluator with chart argument, plus optionally mesh elements argument.",
					getName(fmlShapeEvaluator).c_str(), name.c_str());
				return_code = 0;
			}
			else
			{
				FieldmlHandleType shapeEvaluatorType = Fieldml_GetObjectType(fmlSession, fmlShapeEvaluator);
				switch (shapeEvaluatorType)
				{
				case FHT_EXTERNAL_EVALUATOR:
					{
						// Case 1. single recognised shape external evaluator = all elements same shape
						const_shape_type = getElementShapeFromFieldmlName(getName(fmlShapeEvaluator).c_str());
						if (const_shape_type == CMZN_ELEMENT_SHAPE_TYPE_INVALID)
						{
							display_message(ERROR_MESSAGE, "Read FieldML:  Unrecognised element shape evaluator %s for mesh type %s.",
								getName(fmlShapeEvaluator).c_str(), name.c_str());
							return_code = 0;
							break;
						}
					} break;
				case FHT_PIECEWISE_EVALUATOR:
					{
						FmlObjectHandle fmlIndexArgument = Fieldml_GetIndexEvaluator(fmlSession, fmlShapeEvaluator, 1);
						FmlObjectHandle fmlElementToShapeParameter = FML_INVALID_OBJECT_HANDLE;
						if (fmlIndexArgument == fmlElementsArgument)
						{
							// Case 2. piecewise over elements, directly mapping to recognised shape external evaluators
							// nothing more to do
						}
						else if ((1 != Fieldml_GetBindCount(fmlSession, fmlShapeEvaluator)) ||
							(Fieldml_GetBindArgument(fmlSession, fmlShapeEvaluator, 1) != fmlIndexArgument) ||
							(FML_INVALID_OBJECT_HANDLE == (fmlElementToShapeParameter = Fieldml_GetBindEvaluator(fmlSession, fmlShapeEvaluator, 1))) ||
							(FHT_PARAMETER_EVALUATOR != Fieldml_GetObjectType(fmlSession, fmlElementToShapeParameter)) ||
							(Fieldml_GetValueType(fmlSession, fmlElementToShapeParameter) !=
								Fieldml_GetValueType(fmlSession, fmlIndexArgument)))
						{
							display_message(ERROR_MESSAGE, "Read FieldML:  Shape evaluator %s for mesh type %s has unrecognised piecewise form.",
								getName(fmlShapeEvaluator).c_str(), name.c_str());
							return_code = 0;
						}
						else
						{
							// Case 3. piecewise over 'shape ensemble', indirectly mapping from parameters mapping from element
							elementShapeParameters = HDsMapInt(this->getEnsembleParameters(fmlElementToShapeParameter));
							if (!elementShapeParameters)
							{
								display_message(ERROR_MESSAGE, "Read FieldML:  Invalid element to shape parameters %s for shape evaluator %s of mesh type %s.",
									getName(fmlElementToShapeParameter).c_str(), getName(fmlShapeEvaluator).c_str(), name.c_str());
							}
						}
					} break;
				default:
					display_message(ERROR_MESSAGE, "Read FieldML:  Shape evaluator %s for mesh type %s has unrecognised form.",
						getName(fmlShapeEvaluator).c_str(), name.c_str());
					return_code = 0;
					break;
				}
			}
		}

		// create elements in the mesh of given dimension

		cmzn_mesh_id mesh = cmzn_fieldmodule_find_mesh_by_dimension(field_module, meshDimension);
		cmzn_elementtemplate_id elementtemplate = cmzn_mesh_create_elementtemplate(mesh);

		FmlObjectHandle fmlLastElementShapeEvaluator = FML_INVALID_OBJECT_HANDLE;
		cmzn_element_shape_type last_shape_type = CMZN_ELEMENT_SHAPE_TYPE_INVALID;

		HDsMapIndexing elementShapeParametersIndexing(elementShapeParameters ? elementShapeParameters->createIndexing() : 0);

		DsLabelIterator *elementsLabelIterator = elementsLabels->createLabelIterator();
		if (!elementsLabelIterator)
			return_code = 0;
		else
		{
			while (elementsLabelIterator->increment())
			{
				int elementIdentifier = elementsLabelIterator->getIdentifier();
				cmzn_element_shape_type shape_type = const_shape_type;
				if (const_shape_type == CMZN_ELEMENT_SHAPE_TYPE_INVALID)
				{
					int shapeIdentifier = elementIdentifier;
					if (elementShapeParameters && (
						(CMZN_OK != elementShapeParametersIndexing->setEntry(*elementsLabelIterator)) ||
						(CMZN_OK != elementShapeParameters->getValues(*elementShapeParametersIndexing, 1, &shapeIdentifier))))
					{
						display_message(ERROR_MESSAGE, "Read FieldML:  Failed to map shape of element %d in mesh type %s.",
							elementIdentifier, name.c_str());
						return_code = 0;
						break;
					}
					else
					{
						FmlObjectHandle fmlElementShapeEvaluator =
							Fieldml_GetElementEvaluator(fmlSession, fmlShapeEvaluator, shapeIdentifier, /*allowDefault*/1);
						if (fmlElementShapeEvaluator == fmlLastElementShapeEvaluator)
						{
							shape_type = last_shape_type;
						}
						else
						{
							shape_type = getElementShapeFromFieldmlName(getName(fmlElementShapeEvaluator).c_str());
							fmlLastElementShapeEvaluator = fmlElementShapeEvaluator;
						}
						if (shape_type == CMZN_ELEMENT_SHAPE_TYPE_INVALID)
						{
							display_message(ERROR_MESSAGE, "Read FieldML:  Could not get shape of element %d in mesh type %s.",
								elementIdentifier, name.c_str());
							return_code = 0;
							break;
						}
					}
				}
				if (shape_type != last_shape_type)
				{
					if (!(cmzn_elementtemplate_set_element_shape_type(elementtemplate, shape_type)))
					{
						return_code = 0;
						break;
					}
					last_shape_type = shape_type;
				}
				if (!cmzn_mesh_define_element(mesh, elementIdentifier, elementtemplate))
				{
					return_code = 0;
					break;
				}
			}
		}
		cmzn::Deaccess(elementsLabelIterator);
		cmzn_elementtemplate_destroy(&elementtemplate);
		cmzn_mesh_destroy(&mesh);
	}
	return return_code;
}

bool FieldMLReader::isValueTypeReal1D(FmlObjectHandle fmlEvaluator)
{
	const FmlObjectHandle fmlEvaluatorValueType = Fieldml_GetValueType(this->fmlSession, fmlEvaluator);
	if (FHT_CONTINUOUS_TYPE != Fieldml_GetObjectType(this->fmlSession, fmlEvaluatorValueType))
		return false;
	return (0 == this->getDeclaredName(fmlEvaluatorValueType).compare("real.1d"));
}

int FieldMLReader::readConstantIndex(FmlObjectHandle fmlEnsembleType, FmlObjectHandle fmlConstantEvaluator)
{
	if ((FHT_CONSTANT_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlConstantEvaluator))
		|| (Fieldml_GetValueType(this->fmlSession, fmlConstantEvaluator) != fmlEnsembleType))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Expected evaluator %s to be a constant evaluator of type %s",
			this->getName(fmlConstantEvaluator).c_str(), this->getName(fmlEnsembleType).c_str());
		return 0;
	}
	char *valueString = Fieldml_GetConstantEvaluatorValueString(this->fmlSession, fmlConstantEvaluator);
	trim_string_in_place(valueString);
	int index = atoi(valueString);
	if ((0 == index) || (strspn(valueString, "0123456789") != strlen(valueString)))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Invalid index %s in constant evaluator %s",
			index, this->getName(fmlConstantEvaluator).c_str());
		index = 0;
	}
	Fieldml_FreeString(valueString);
	return index;
}

/** Read standard FieldML basis interpolator and convert into FE_basis and supporting data.
  * @param fmlChartArgument  On success, set to generic chart argument for mesh/basis dimension.
  * @param fmlParametersArgument  On success, set to parameters argument for basis.
  * @param swizzle  On success, set to optional swizzle array giving source parameter
  * index (1-based) for each 0-based basis function, or 0 if no swizzle. This supports
  * bases which are equivalent to those used in zinc but with different parameter ordering.
  * @return  Non-acccessed basis or 0 if invalid. */
FE_basis *FieldMLReader::readBasisInterpolator(FmlObjectHandle fmlInterpolator, FmlObjectHandle& fmlChartArgument,
	FmlObjectHandle& fmlParametersArgument, const int*& swizzle)
{
	fmlChartArgument = FML_INVALID_OBJECT_HANDLE;
	fmlParametersArgument = FML_INVALID_OBJECT_HANDLE;
	swizzle = 0;
	const std::string interpolatorName = this->getDeclaredName(fmlInterpolator);
	const std::string interpolatorLocalName = this->getName(fmlInterpolator);
	const char *interpolator_name = interpolatorName.c_str();
	int basis_index = -1;
	for (int i = 0; i < numLibraryBases; ++i)
	{
		if (0 == strcmp(interpolator_name, libraryBases[i].fieldmlBasisEvaluatorName))
		{
			basis_index = i;
			break;
		}
	}
	if (basis_index < 0)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Unrecognised basis interpolator %s (local name %s).",
			interpolator_name, interpolatorLocalName.c_str());
		return 0;
	}
	const int meshDimension = this->mesh->getDimension();
	cmzn_elementbasis_id elementBasis = cmzn_fieldmodule_create_elementbasis(this->field_module, meshDimension,
		libraryBases[basis_index].functionType[0]);
	if (!libraryBases[basis_index].homogeneous)
	{
		for (int dimension = 2; dimension <= meshDimension; ++dimension)
		{
			cmzn_elementbasis_set_function_type(elementBasis, dimension,
				libraryBases[basis_index].functionType[dimension - 1]);
		}
	}
	FE_basis *basis = cmzn_elementbasis_get_FE_basis(elementBasis); // not accessed
	cmzn_elementbasis_destroy(&elementBasis);
	if (!basis)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Failed to create basis for interpolator %s (local name %s).",
			interpolator_name, interpolatorLocalName.c_str());
		return 0;
	}
	swizzle = libraryBases[basis_index].zinc_to_source_parameter_swizzle;

	// Note: external evaluator arguments are assumed to be 'used'
	const int interpolatorArgumentCount = Fieldml_GetArgumentCount(this->fmlSession, fmlInterpolator, /*isBound*/0, /*isUsed*/1);
	if (interpolatorArgumentCount != 2)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Interpolator %s (local name %s) has %d argument(s); 2 are expected (chart and parameters).",
			interpolator_name, interpolatorLocalName.c_str(), interpolatorArgumentCount);
		return 0;
	}
	for (int index = 1; index <= interpolatorArgumentCount; ++index)
	{
		FmlObjectHandle fmlArgument = Fieldml_GetArgument(this->fmlSession, fmlInterpolator, index, /*isBound*/0, /*isUsed*/1);
		std::string argName = this->getDeclaredName(fmlArgument);
		if (0 == argName.compare(libraryChartArgumentNames[meshDimension]))
		{
			if (fmlChartArgument != FML_INVALID_OBJECT_HANDLE)
			{
				fmlChartArgument = FML_INVALID_OBJECT_HANDLE;
				break;
			}
			fmlChartArgument = fmlArgument;
		}
		else
		{
			fmlParametersArgument = fmlArgument;
		}
	}
	if ((FML_INVALID_OBJECT_HANDLE == fmlChartArgument) ||
		(FML_INVALID_OBJECT_HANDLE == fmlParametersArgument))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Interpolator %s (local name %s) is not a standard basis interpolator over %s.",
			interpolator_name, interpolatorLocalName.c_str(), libraryChartArgumentNames[meshDimension]);
		return 0;
	}
	return basis;
}

bool FieldMLReader::readElementFieldTemplateParameter(FieldmlEft &fieldmlEft,
	int functionNumber, FmlObjectHandle fmlParameterEvaluator)
{
	/*
	<ReferenceEvaluator name = "mesh3d.eft1.nodeparameters.node1.value.v1" evaluator = "mesh3d.eft1.nodeparameters.argument" valueType = "real.1d">
		<Bindings>
			<Bind argument = "mesh3d.eft1.nodes.argument" source = "mesh3d.eft1.nodes.1"/>
			<Bind argument = "node_derivatives.argument" source = "node_derivatives.value"/>
			<Bind argument = "node_versions.argument" source = "node_versions.1"/>
		</Bindings>
	</ReferenceEvaluator>
	*/
	const std::string parameterEvaluatorName = this->getName(fmlParameterEvaluator);
	if ((FHT_REFERENCE_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlParameterEvaluator)) ||
		(!this->isValueTypeReal1D(fmlParameterEvaluator)))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Element field template parameter evaluator %s is not a real-valued reference evaluator",
			parameterEvaluatorName.c_str());
		return false;
	}
	FE_element_field_template *eft = fieldmlEft.getEft();

	// Discover or check source evaluator is EFT node parameters
	// Future: support element-based, field-based
	FmlObjectHandle fmlSourceEvaluator = Fieldml_GetReferenceSourceEvaluator(this->fmlSession, fmlParameterEvaluator);
	if (FML_INVALID_OBJECT_HANDLE != fieldmlEft.getFmlNodeParametersArgument())
	{
		if (fmlSourceEvaluator != fieldmlEft.getFmlNodeParametersArgument())
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Element field template parameter evaluator %s has inconsistent eft node parameters source evaluator",
				parameterEvaluatorName.c_str());
			return false;
		}
	}
	else
	{
		/*
		<ArgumentEvaluator name = "mesh3d.eft1.nodeparameters.argument" valueType = "real.1d">
			<Arguments>
				<Argument name = "node_derivatives.argument"/>
				<Argument name = "node_versions.argument"/>
				<Argument name = "mesh3d.eft1.nodes.argument"/>
			</Arguments>
		</ArgumentEvaluator>
		*/
		if ((FHT_ARGUMENT_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlSourceEvaluator))
			|| (!this->isValueTypeReal1D(fmlSourceEvaluator))
			|| (Fieldml_GetArgumentCount(this->fmlSession, fmlSourceEvaluator, /*isBound*/false, /*isUsed*/true) != 3))
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Element field template parameter evaluator %s source evaluator %s is not a valid EFT node parameters argument",
				parameterEvaluatorName.c_str(), this->getName(fmlSourceEvaluator).c_str());
			return false;
		}
		int nodeDerivativesArgumentCount = 0;
		int nodeVersionsArgumentCount = 0;
		FmlObjectHandle fmlLocalNodesArgument = FML_INVALID_OBJECT_HANDLE;
		for (int index = 1; index <= 3; ++index)
		{
			FmlObjectHandle fmlArgumentArg = Fieldml_GetArgument(this->fmlSession, fmlSourceEvaluator, index, /*isBound*/false, /*isUsed*/true);
			if (fmlArgumentArg == this->fmlNodeDerivativesArgument)
				++nodeDerivativesArgumentCount;
			else if (fmlArgumentArg == this->fmlNodeVersionsArgument)
				++nodeVersionsArgumentCount;
			else
				fmlLocalNodesArgument = fmlArgumentArg;
		}
		if ((fmlLocalNodesArgument == FML_INVALID_OBJECT_HANDLE)
			|| (1 != nodeDerivativesArgumentCount)
			|| (1 != nodeVersionsArgumentCount))
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Element field template parameter evaluator %s source evaluator %s "
				"must be an argument evaluator with local nodes, global derivatives and global versions arguments bound to it",
				parameterEvaluatorName.c_str(), this->getName(fmlSourceEvaluator).c_str());
			return false;
		}
		/*
		<EnsembleType name = "mesh3d.eft1.nodes">
			<Members>
				<MemberRange min = "1" max = "8"/>
			</Members>
		</EnsembleType>
		<ArgumentEvaluator name = "mesh3d.eft1.nodes.argument" valueType = "mesh3d.eft1.nodes"/>
		*/
		FmlObjectHandle fmlLocalNodes = Fieldml_GetValueType(this->fmlSession, fmlLocalNodesArgument);
		int localNodeCount = 0;
		const bool validLocalNodes = (FHT_ARGUMENT_EVALUATOR == Fieldml_GetObjectType(this->fmlSession, fmlLocalNodesArgument))
			&& (FHT_ENSEMBLE_TYPE == Fieldml_GetObjectType(this->fmlSession, fmlLocalNodes))
			&& (FML_ENSEMBLE_MEMBER_RANGE == Fieldml_GetEnsembleMembersType(this->fmlSession, fmlLocalNodes))
			&& (1 == Fieldml_GetEnsembleMembersMin(this->fmlSession, fmlLocalNodes))
			&& (1 <= (localNodeCount = Fieldml_GetEnsembleMembersMax(this->fmlSession, fmlLocalNodes)))
			&& (1 == Fieldml_GetEnsembleMembersStride(this->fmlSession, fmlLocalNodes));
		if (!validLocalNodes)
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Element field template parameter evaluator %s source evaluator %s "
				"argument %s is not a valid local nodes argument",
				parameterEvaluatorName.c_str(), this->getName(fmlSourceEvaluator).c_str(), this->getName(fmlLocalNodesArgument).c_str());
			return false;
		}
		if (CMZN_OK != eft->setNumberOfLocalNodes(localNodeCount))
			return false;
		fieldmlEft.setFmlLocalNodes(fmlLocalNodes);
		fieldmlEft.setFmlLocalNodesArgument(fmlLocalNodesArgument);
		fieldmlEft.setFmlNodeParametersArgument(fmlSourceEvaluator);
	}
	// now determine node parameter map
	FmlObjectHandle fmlLocalNodesArgument = fieldmlEft.getFmlLocalNodesArgument();
	FmlObjectHandle fmlLocalNodeEvaluator = Fieldml_GetBindByArgument(this->fmlSession, fmlParameterEvaluator, fmlLocalNodesArgument);
	// e.g. <ConstantEvaluator name = "mesh3d.eft1.nodes.1" value = "1" valueType = "mesh3d.eft1.nodes"/>
	const int localNodeIndex = this->readConstantIndex(fieldmlEft.getFmlLocalNodes(), fmlLocalNodeEvaluator);
	if (localNodeIndex == 0)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Element field template parameter evaluator %s local node evaluator %s "
			"bound to argument %s is not a valid node index",
			parameterEvaluatorName.c_str(), this->getName(fmlLocalNodeEvaluator).c_str(), this->getName(fmlLocalNodesArgument).c_str());
		return false;
	}
	// e.g. <ConstantEvaluator name="node_derivatives.value" value="1" valueType="node_derivatives"/>
	FmlObjectHandle fmlNodeDerivativeEvaluator = Fieldml_GetBindByArgument(this->fmlSession, fmlParameterEvaluator, this->fmlNodeDerivativesArgument);
	const int derivativeIndex = this->readConstantIndex(this->fmlNodeDerivativesType, fmlNodeDerivativeEvaluator);
	if (derivativeIndex == 0)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Element field template parameter evaluator %s node derivative evaluator %s "
			"bound to argument %s is not a valid derivative index",
			parameterEvaluatorName.c_str(), this->getName(fmlNodeDerivativeEvaluator).c_str(), this->getName(fmlNodeDerivativesArgument).c_str());
		return false;
	}
	// e.g. <ConstantEvaluator name="node_versions.1" value="1" valueType="node_versions"/>
	FmlObjectHandle fmlNodeVersionEvaluator = Fieldml_GetBindByArgument(this->fmlSession, fmlParameterEvaluator, this->fmlNodeVersionsArgument);
	const int versionIndex = this->readConstantIndex(this->fmlNodeVersionsType, fmlNodeVersionEvaluator);
	if (versionIndex == 0)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Element field template parameter evaluator %s node version evaluator %s "
			"bound to argument %s is not a valid version index",
			parameterEvaluatorName.c_str(), this->getName(fmlNodeVersionEvaluator).c_str(), this->getName(fmlNodeVersionsArgument).c_str());
		return false;
	}
	const cmzn_node_value_label nodeValueLabel = static_cast<cmzn_node_value_label>(CMZN_NODE_VALUE_LABEL_VALUE + derivativeIndex - 1);
	if (CMZN_OK != eft->setTermNodeParameter(functionNumber, /*term*/0, localNodeIndex - 1, nodeValueLabel, versionIndex - 1))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Failed to set function %d term %d node parameter for element field template parameter evaluator %s",
			functionNumber + 1, 1, parameterEvaluatorName.c_str());
		return false;
	}
	return true;
}

FieldmlEft *FieldMLReader::readElementFieldTemplate(FmlObjectHandle fmlEvaluator)
{
	/*
	<ReferenceEvaluator name = "mesh3d.eft1" evaluator = "interpolator.3d.unit.trilinearLagrange" valueType = "real.1d">
		<Bindings>
			<Bind argument = "parameters.3d.unit.trilinearLagrange.argument" source = "mesh3d.eft1.parameters"/>
		</Bindings>
	</ReferenceEvaluator>
	*/
	const std::string evaluatorName = this->getName(fmlEvaluator);
	if ((FHT_REFERENCE_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlEvaluator))
		|| (!this->isValueTypeReal1D(fmlEvaluator)))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Element field template evaluator %s is not a real-valued reference evaluator",
			evaluatorName.c_str());
		return 0;
	}
	const FmlObjectHandle fmlInterpolator = Fieldml_GetReferenceSourceEvaluator(this->fmlSession, fmlEvaluator);
	FmlObjectHandle fmlChartArgument = FML_INVALID_OBJECT_HANDLE;
	FmlObjectHandle fmlParametersArgument = FML_INVALID_OBJECT_HANDLE;
	const int *swizzle = 0;
	FE_basis *basis = this->readBasisInterpolator(fmlInterpolator, fmlChartArgument, fmlParametersArgument, swizzle);
	if (!basis)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Element field template evaluator %s source evaluator is not a valid basis interpolator",
			evaluatorName.c_str());
		return 0;
	}
	/*
	<AggregateEvaluator name = "mesh3d.eft1.parameters" valueType = "parameters.3d.unit.trilinearLagrange">
		<Bindings>
			<BindIndex argument = "parameters.3d.unit.trilinearLagrange.component.argument" indexNumber = "1"/>
		</Bindings>
		<ComponentEvaluators>
			<ComponentEvaluator component = "1" evaluator = "mesh3d.eft1.nodeparameters.node1.value.v1"/>
			<ComponentEvaluator component = "2" evaluator = "mesh3d.eft1.nodeparameters.node2.value.v1"/>
			<ComponentEvaluator component = "3" evaluator = "mesh3d.eft1.nodeparameters.node3.value.v1"/>
			<ComponentEvaluator component = "4" evaluator = "mesh3d.eft1.nodeparameters.node4.value.v1"/>
			<ComponentEvaluator component = "5" evaluator = "mesh3d.eft1.nodeparameters.node5.value.v1"/>
			<ComponentEvaluator component = "6" evaluator = "mesh3d.eft1.nodeparameters.node6.value.v1"/>
			<ComponentEvaluator component = "7" evaluator = "mesh3d.eft1.nodeparameters.node7.value.v1"/>
			<ComponentEvaluator component = "8" evaluator = "mesh3d.eft1.nodeparameters.node8.value.v1"/>
		</ComponentEvaluators>
	</AggregateEvaluator>
	*/
	// check parameters argument type is continuous vector with correct number of components (parameters)
	FmlObjectHandle fmlParametersArgumentValueType = Fieldml_GetValueType(this->fmlSession, fmlParametersArgument);
	if (FHT_CONTINUOUS_TYPE != Fieldml_GetObjectType(this->fmlSession, fmlParametersArgumentValueType))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Element field template evaluator %s basis interpolator parameters %s is not continuous type",
			evaluatorName.c_str(), this->getName(fmlParametersArgument).c_str());
		return 0;
	}
	const int parameterCount = Fieldml_GetTypeComponentCount(this->fmlSession, fmlParametersArgumentValueType);
	const int functionCount = FE_basis_get_number_of_functions(basis);
	if (parameterCount != functionCount)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Element field template evaluator %s basis interpolator parameters %s has %d components; %d expected for basis",
			evaluatorName.c_str(), this->getName(fmlParametersArgument).c_str(), parameterCount, functionCount);
		return 0;
	}
	const FmlObjectHandle fmlParametersArgumentComponentType = Fieldml_GetTypeComponentEnsemble(this->fmlSession, fmlParametersArgumentValueType);

	const int evaluatorBindCount = Fieldml_GetBindCount(this->fmlSession, fmlEvaluator);
	// only bind parameters here; mesh chart is unbound in generic element field template
	if (1 != evaluatorBindCount)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Evaluator %s has %d bindings; only expect to bind to parameters in generic element field template.",
			evaluatorName.c_str(), evaluatorBindCount);
		return 0;
	}
	const FmlObjectHandle fmlElementParametersEvaluator = Fieldml_GetBindByArgument(this->fmlSession, fmlEvaluator, fmlParametersArgument);
	if (fmlElementParametersEvaluator == FML_INVALID_OBJECT_HANDLE)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Evaluator %s does not bind to parameters argument %s for basis interpolator.",
			evaluatorName.c_str(), getDeclaredName(fmlParametersArgument).c_str());
		return 0;
	}
	const FmlObjectHandle fmlElementParametersEvaluatorValueType = Fieldml_GetValueType(this->fmlSession, fmlElementParametersEvaluator);
	std::string elementParametersName = getName(fmlElementParametersEvaluator);
	// check parameter argument produces expected continuous parameter vector expected for basis
	if (fmlElementParametersEvaluatorValueType != fmlParametersArgumentValueType)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Element field template evaluator %s basis interpolator parameters %s are not of expected type %s",
			evaluatorName.c_str(), elementParametersName.c_str(), this->getName(fmlParametersArgumentValueType).c_str());
		return 0;
	}

	if ((Fieldml_GetObjectType(this->fmlSession, fmlElementParametersEvaluator) != FHT_AGGREGATE_EVALUATOR) ||
		(1 != Fieldml_GetIndexEvaluatorCount(this->fmlSession, fmlElementParametersEvaluator)))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Expect evaluator %s parameter source %s to be an AggregateEvaluator with 1 index",
			evaluatorName.c_str(), elementParametersName.c_str());
		return 0;
	}
	const FmlObjectHandle fmlElementParametersIndexArgument = Fieldml_GetIndexEvaluator(this->fmlSession, fmlElementParametersEvaluator, 1);
	FmlObjectHandle fmlElementParametersIndexArgumentValueType = Fieldml_GetValueType(this->fmlSession, fmlElementParametersIndexArgument);
	if ((Fieldml_GetObjectType(this->fmlSession, fmlElementParametersIndexArgument) != FHT_ARGUMENT_EVALUATOR) ||
		(fmlElementParametersIndexArgumentValueType != fmlParametersArgumentComponentType))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Aggregate %s index %s is not of expected parameter component argument type %s",
			elementParametersName.c_str(), getName(fmlElementParametersIndexArgument).c_str(), getName(fmlParametersArgumentComponentType).c_str());
		return 0;
	}

	FE_element_field_template *eft = FE_element_field_template::create(this->mesh, basis);
	auto fieldmlEft = FieldmlEft::create(eft);
	FE_element_field_template::deaccess(eft);
	if (!fieldmlEft)
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::readElementFieldTemplate.  Failed to create element field template");
		return 0;
	}
	fieldmlEft->setFmlChartArgument(fmlChartArgument);

	for (int f = 0; f < functionCount; ++f)
	{
		const int parameterIndex = swizzle ? swizzle[f] : f + 1;
		// GRC consider allowDefault:
		FmlObjectHandle fmlParameterEvaluator = Fieldml_GetElementEvaluator(this->fmlSession, fmlElementParametersEvaluator, parameterIndex, /*allowDefault*/1);
		if (FML_INVALID_OBJECT_HANDLE == fmlParameterEvaluator)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Missing element parameter %d in parameters %d for element field template %s.",
				parameterIndex, getName(fmlElementParametersEvaluator).c_str(), evaluatorName.c_str());
			FieldmlEft::deaccess(fieldmlEft);
			return 0;
		}
		if (!this->readElementFieldTemplateParameter(*fieldmlEft, f, fmlParameterEvaluator))
		{
			FieldmlEft::deaccess(fieldmlEft);
			return 0;
		}
	}

	// Future: read scale factor metadata

	if (!fieldmlEft->validateAndMergeEftIntoMesh())
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Failed to merge element field template %s into mesh",
			evaluatorName.c_str());
		FieldmlEft::deaccess(fieldmlEft);
		return 0;
	}
	return fieldmlEft;
}

bool FieldMLReader::isValidNodeParametersArgument(FmlObjectHandle fmlArgument)
{
	/*
	<ArgumentEvaluator name="nodes.parameters" valueType="real.1d">
		<Arguments>
			<Argument name="nodes.argument"/>
			<Argument name="node_derivatives.argument"/>
			<Argument name="node_versions.argument"/>
		</Arguments>
	</ArgumentEvaluator>
	*/
	std::string argumentName = this->getName(fmlArgument);
	if ((Fieldml_GetObjectType(this->fmlSession, fmlArgument) != FHT_ARGUMENT_EVALUATOR)
		|| (!this->isValueTypeReal1D(fmlArgument)))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Evaluator %s is not a real-valued argument as expected for node parameters",
			argumentName.c_str());
		return false;
	}
	// GRC check isUsed value true
	const int argumentCount = Fieldml_GetArgumentCount(this->fmlSession, fmlArgument, /*isBound*/false, /*isUsed*/true);
	int nodesArgumentCount = 0;
	int nodeDerivativesArgumentCount = 0;
	int nodeVersionsArgumentCount = 0;
	for (int index = 1; index <= argumentCount; ++index)
	{
		FmlObjectHandle fmlArgumentArg = Fieldml_GetArgument(this->fmlSession, fmlArgument, index, /*isBound*/false, /*isUsed*/true);
		if (fmlArgumentArg == this->fmlNodesArgument)
			++nodesArgumentCount;
		else if (fmlArgumentArg == this->fmlNodeDerivativesArgument)
			++nodeDerivativesArgumentCount;
		else if (fmlArgumentArg == this->fmlNodeVersionsArgument)
			++nodeVersionsArgumentCount;
	}
	if ((3 != argumentCount) || (1 != nodesArgumentCount) || (1 != nodeDerivativesArgumentCount) || (1 != nodeVersionsArgumentCount))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Node parameter evaluator %s must have 3 arguments: nodes, node_derivatives and node_versions",
			argumentName.c_str());
		return false;
	}
	return true;
}

bool FieldMLReader::isValidLocalToGlobalNodeMap(FmlObjectHandle fmlLocalToGlobalNodeMap, FmlObjectHandle fmlLocalNodesArgument)
{
	/* Example with 2 dense indexes; can alternatively be sparse in elements only
	<ParameterEvaluator name = "mesh3d.eft1.localtoglobalnodes" valueType = "nodes">
		<DenseArrayData data = "mesh3d.eft1.localtoglobalnodes.data.source">
			<DenseIndexes>
				<IndexEvaluator evaluator = "mesh3d.argument.elements"/>
				<IndexEvaluator evaluator = "mesh3d.eft1.nodes.argument"/>
			</DenseIndexes>
		</DenseArrayData>
	</ParameterEvaluator>
	*/
	std::string mapName = this->getName(fmlLocalToGlobalNodeMap);
	if ((Fieldml_GetObjectType(this->fmlSession, fmlLocalToGlobalNodeMap) != FHT_PARAMETER_EVALUATOR)
		|| (Fieldml_GetValueType(this->fmlSession, fmlLocalToGlobalNodeMap) != this->fmlNodesType))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Evaluator %s is not a nodes ensemble valued parameter evaluator as needed for a local to global node map",
			mapName.c_str());
		return false;
	}
	FieldmlDataDescriptionType dataDescription = Fieldml_GetParameterDataDescription(this->fmlSession, fmlLocalToGlobalNodeMap);
	if ((dataDescription != FML_DATA_DESCRIPTION_DENSE_ARRAY) &&
		(dataDescription != FML_DATA_DESCRIPTION_DOK_ARRAY))
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Unknown data description for local to global node map %s; must be dense array or DOK array", mapName.c_str());
		return false;
	}
	const int sparseIndexCount = (dataDescription == FML_DATA_DESCRIPTION_DOK_ARRAY) ?
		Fieldml_GetParameterIndexCount(this->fmlSession, fmlLocalToGlobalNodeMap, /*isSparse*/1) : 0;
	const int denseIndexCount = Fieldml_GetParameterIndexCount(this->fmlSession, fmlLocalToGlobalNodeMap, /*isSparse*/0);
	if ((denseIndexCount < 1) || ((denseIndexCount + sparseIndexCount) != 2))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Local to global node map %s must have two indexes and local node index must be dense",
			mapName.c_str());
		return false;
	}
	FmlObjectHandle fmlIndexEvaluator1 = Fieldml_GetParameterIndexEvaluator(this->fmlSession, fmlLocalToGlobalNodeMap, 1, /*isSparse*/false);
	FmlObjectHandle fmlIndexEvaluator2 = (denseIndexCount == 2) ?
		Fieldml_GetParameterIndexEvaluator(this->fmlSession, fmlLocalToGlobalNodeMap, 2, /*isSparse*/false) :
		Fieldml_GetParameterIndexEvaluator(this->fmlSession, fmlLocalToGlobalNodeMap, 1, /*isSparse*/true);
	if ((fmlIndexEvaluator1 != fmlLocalNodesArgument)
		|| (fmlIndexEvaluator2 != this->fmlElementsArgument))
	{
		if ((denseIndexCount != 2)
			|| (fmlIndexEvaluator2 != fmlLocalNodesArgument)
			|| (fmlIndexEvaluator1 != this->fmlElementsArgument))
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Local to global node map %s does not have two indexes with local node index dense, and element index dense or sparse",
				mapName.c_str());
			return false;
		}
	}
	return true;
}

/** Legacy Lagrange formulation: no derivatives or versions, procedural aggregate of basis parameters.
  * Have already checked this is a real-valued reference evaluator with 2 binds */
MeshElementEvaluator* FieldMLReader::readMeshElementEvaluatorLegacy(FmlObjectHandle fmlEvaluator)
{
	/*
	<ReferenceEvaluator name="mesh3d.trilinearLagrange" evaluator="trilinearLagrange.interpolator" valueType="real.1d">
		<Bindings>
			<Bind argument="chart.3d.argument" source="mesh3d.argument.xi" />
			<Bind argument="parameters.3d.unit.trilinearLagrange.argument" source="mesh3d.trilinearLagrange.parameters" />
		</Bindings>
	</ReferenceEvaluator>
	*/
	const std::string evaluatorName = this->getName(fmlEvaluator);
	const FmlObjectHandle fmlInterpolator = Fieldml_GetReferenceSourceEvaluator(this->fmlSession, fmlEvaluator);
	FmlObjectHandle fmlChartArgument = FML_INVALID_OBJECT_HANDLE;
	FmlObjectHandle fmlParametersArgument = FML_INVALID_OBJECT_HANDLE;
	const int *swizzle = 0;
	FE_basis *basis = this->readBasisInterpolator(fmlInterpolator, fmlChartArgument, fmlParametersArgument, swizzle);
	if (!basis)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Legacy mesh element evaluator %s source evaluator is not a valid basis interpolator",
			evaluatorName.c_str());
		return 0;
	}
	if (Fieldml_GetBindByArgument(this->fmlSession, fmlEvaluator, fmlChartArgument) != this->fmlMeshChartArgument)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader.  Legacy mesh element evaluator %s does not bind the mesh chart, hence is of invalid form",
			evaluatorName.c_str());
		return 0;
	}
	FmlObjectHandle fmlParametersEvaluator = Fieldml_GetBindByArgument(this->fmlSession, fmlEvaluator, fmlParametersArgument);
	if (fmlParametersEvaluator == FML_INVALID_OBJECT_HANDLE)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Legacy mesh element evaluator %s does not bind to basis parameters %d, hence is of invalid form",
			evaluatorName.c_str(), this->getName(fmlParametersArgument).c_str());
		return 0;
	}
	/*
	<AggregateEvaluator name="mesh3d.trilinearLagrange.parameters" valueType="parameters.3d.unit.trilinearLagrange" >
		<Bindings>
			<BindIndex argument="parameters.3d.unit.trilinearLagrange.component.argument" indexNumber="1" />
			<Bind argument="nodes.argument" source="mesh3d.trilinearLagrange.localtoglobalnodes" />
		</Bindings>
		<ComponentEvaluators default="nodes.parameters" />
	</AggregateEvaluator>
	*/
	FmlObjectHandle fmlLocalNodesArgument = FML_INVALID_OBJECT_HANDLE;
	FmlObjectHandle fmlLocalToGlobalNodeMap = FML_INVALID_OBJECT_HANDLE;
	if ((fmlParametersEvaluator == FML_INVALID_OBJECT_HANDLE)
		|| (Fieldml_GetObjectType(this->fmlSession, fmlParametersEvaluator) != FHT_AGGREGATE_EVALUATOR)
		|| (1 != Fieldml_GetIndexEvaluatorCount(this->fmlSession, fmlParametersEvaluator))
		|| ((fmlLocalNodesArgument = Fieldml_GetIndexEvaluator(this->fmlSession, fmlParametersEvaluator, 1))
			== FML_INVALID_OBJECT_HANDLE))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Legacy mesh element evaluator %s parameter source %d "
			"is not an aggregate evaluator indexed by local nodes ensemble argument",
			evaluatorName.c_str(), this->getName(fmlParametersEvaluator).c_str());
		return 0;
	}
	FmlObjectHandle fmlParameterComponentsType =
		Fieldml_GetTypeComponentEnsemble(this->fmlSession, Fieldml_GetValueType(this->fmlSession, fmlParametersEvaluator));
	if ((fmlParameterComponentsType == FML_INVALID_OBJECT_HANDLE)
		|| (Fieldml_GetValueType(this->fmlSession, fmlLocalNodesArgument) != fmlParameterComponentsType)
		|| (1 != Fieldml_GetBindCount(this->fmlSession, fmlParametersEvaluator))
		|| ((fmlLocalToGlobalNodeMap = Fieldml_GetBindByArgument(this->fmlSession, fmlParametersEvaluator, this->fmlNodesArgument))
			== FML_INVALID_OBJECT_HANDLE)
		|| (!this->isValidLocalToGlobalNodeMap(fmlLocalToGlobalNodeMap, fmlLocalNodesArgument)))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Legacy mesh element evaluator %s parameter source %s does not bind value local to global node map to nodes argument",
			evaluatorName.c_str(), this->getName(fmlParametersEvaluator).c_str());
	}
	FmlObjectHandle fmlDefaultEvaluator = Fieldml_GetDefaultEvaluator(this->fmlSession, fmlParametersEvaluator);
	if ((0 != Fieldml_GetEvaluatorCount(this->fmlSession, fmlParametersEvaluator)) // support only default
		|| (fmlDefaultEvaluator == FML_INVALID_OBJECT_HANDLE))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Legacy mesh element evaluator %s parameter source %s cannot be read as default evaluator only supported by reader.",
			evaluatorName.c_str(), this->getName(fmlParametersEvaluator).c_str());
		return 0;
	}
	if (fmlDefaultEvaluator != this->fmlNodeParametersArgument)
	{
		if (FML_INVALID_OBJECT_HANDLE == this->fmlNodeParametersArgument)
		{
			// discover legacy node parameters as argument of standard name "nodes.parameters" was not found earlier
			/*
			<ArgumentEvaluator name = "nodes.dofs.argument" valueType = "real.1d">
				<Arguments>
					<Argument name = "nodes.argument" />
				</Arguments>
			</ArgumentEvaluator>
			*/
			// simplification of FieldMLReader::isValidNodeParametersArgument without derivatives or versions
			if ((Fieldml_GetObjectType(this->fmlSession, fmlDefaultEvaluator) != FHT_ARGUMENT_EVALUATOR)
				|| (!this->isValueTypeReal1D(fmlDefaultEvaluator))
				|| (1 != Fieldml_GetArgumentCount(this->fmlSession, fmlDefaultEvaluator, /*isBound*/false, /*isUsed*/true))
				|| (Fieldml_GetArgument(this->fmlSession, fmlDefaultEvaluator, 1, /*isBound*/false, /*isUsed*/true) != this->fmlNodesArgument))
			{
				display_message(ERROR_MESSAGE, "FieldML Reader:  Legacy mesh element evaluator %s parameter source %s is not a valid node parameters argument.",
					evaluatorName.c_str(), this->getName(fmlParametersEvaluator).c_str());
				return 0;
			}
			this->fmlNodeParametersArgument = fmlDefaultEvaluator;
		}
		else
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Legacy mesh element evaluator %s parameter source %s does not have correct node parameters argument as default evaluator.",
				evaluatorName.c_str(), this->getName(fmlParametersEvaluator).c_str());
			return 0;
		}
	}

	FE_element_field_template *eft = FE_element_field_template::create(this->mesh, basis);
	auto fieldmlEft = FieldmlEft::create(eft);
	FE_element_field_template::deaccess(eft);
	if (!fieldmlEft)
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::readMeshElementEvaluatorLegacy.  Failed to create element field template");
		return 0;
	}
	fieldmlEft->setFmlChartArgument(fmlChartArgument);
	fieldmlEft->setFmlLocalNodes(Fieldml_GetValueType(this->fmlSession, fmlLocalNodesArgument));
	fieldmlEft->setFmlLocalNodesArgument(fmlLocalNodesArgument);

	const int parameterCount = Fieldml_GetMemberCount(this->fmlSession, fmlParameterComponentsType);
	const int functionCount = FE_basis_get_number_of_functions(basis);
	if (parameterCount != functionCount)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Legacy mesh element evaluator %s basis interpolator parameters %s has %d components; %d expected for basis",
			evaluatorName.c_str(), this->getName(fmlParametersEvaluator).c_str(), parameterCount, functionCount);
		return 0;
	}
	if (swizzle)
	{
		eft = fieldmlEft->getEft(); // not accessed
		// this only works with Lagrange value-only parameters
		const int functionCount = FE_basis_get_number_of_functions(basis);
		for (int f = 0; f < functionCount; ++f)
		{
			const int localNodeIndex = swizzle[f] - 1;
			if (CMZN_OK != eft->setTermNodeParameter(f, /*term*/0, localNodeIndex, CMZN_NODE_VALUE_LABEL_VALUE, /*version*/1))
			{
				display_message(ERROR_MESSAGE, "FieldMLReader::readMeshElementEvaluatorLegacy.  Failed to set swizzled term node parameter for evaluator %s",
					evaluatorName.c_str());
				FieldmlEft::deaccess(fieldmlEft);
				return 0;
			}
		}
	}
	if (!fieldmlEft->validateAndMergeEftIntoMesh())
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Failed to merge element field template %s into mesh",
			evaluatorName.c_str());
		FieldmlEft::deaccess(fieldmlEft);
		return 0;
	}

	MeshElementEvaluator *meshElementEvaluator = new MeshElementEvaluator(fieldmlEft);
	FieldmlEft::deaccess(fieldmlEft);
	if (meshElementEvaluator)
	{
		meshElementEvaluator->setFmlLocalToGlobalNodeMap(fmlLocalToGlobalNodeMap);
	}
	else
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::readMeshElementEvaluatorLegacy.  Failed to create mesh element evaluator for %s",
			evaluatorName.c_str());
	}
	return meshElementEvaluator;
}

/** New version based on evaluating a source generic element field template.
  * Have already checked this is a real-valued reference evaluator */
MeshElementEvaluator* FieldMLReader::readMeshElementEvaluatorEft(FmlObjectHandle fmlEvaluator)
{
	/*
	<ReferenceEvaluator name="mesh3d.eft1.evaluator" evaluator="mesh3d.eft1" valueType="real.1d">
		<Bindings>
			<Bind argument="chart.3d.argument" source="mesh3d.argument.xi"/>
			<Bind argument="mesh3d.eft1.nodeparameters.argument" source="nodes.parameters"/>
			<Bind argument="nodes.argument" source="mesh3d.eft1.localtoglobalnodes"/>
		</Bindings>
	</ReferenceEvaluator>
	*/
	const std::string evaluatorName = this->getName(fmlEvaluator);
	FmlObjectHandle fmlSourceEvaluator = Fieldml_GetReferenceSourceEvaluator(this->fmlSession, fmlEvaluator);
	FieldmlEft *fieldmlEft = this->readElementFieldTemplate(fmlSourceEvaluator);
	if (!fieldmlEft)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Failed to read mesh element evaluator %s source evaluator %s as element field template",
			evaluatorName.c_str(), this->getName(fmlSourceEvaluator).c_str());
		return 0;
	}

	if (Fieldml_GetBindByArgument(this->fmlSession, fmlEvaluator, fieldmlEft->getFmlChartArgument()) != this->fmlMeshChartArgument)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader.  Mesh element evaluator %s does not bind mesh chart argument %s, hence is of invalid form",
			evaluatorName.c_str(), this->getName(this->fmlMeshChartArgument).c_str());
		FieldmlEft::deaccess(fieldmlEft);
		return 0;
	}
	FmlObjectHandle fmlNodeParametersArgumentToTest = Fieldml_GetBindByArgument(this->fmlSession, fmlEvaluator, fieldmlEft->getFmlNodeParametersArgument());
	if (fmlNodeParametersArgumentToTest == FML_INVALID_OBJECT_HANDLE)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader.  Mesh element evaluator %s does not bind to EFT node parameters %s, hence is of invalid form",
			evaluatorName.c_str(), this->getName(fieldmlEft->getFmlNodeParametersArgument()).c_str());
		FieldmlEft::deaccess(fieldmlEft);
		return 0;
	}
	if ((this->fmlNodeParametersArgument == FML_INVALID_OBJECT_HANDLE)
		&& this->isValidNodeParametersArgument(fmlNodeParametersArgumentToTest))
	{
		this->fmlNodeParametersArgument = fmlNodeParametersArgumentToTest;
	}
	if (fmlNodeParametersArgumentToTest != this->fmlNodeParametersArgument)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader.  Mesh element evaluator %s does not bind global node parameters to EFT node parameters %s, hence is of invalid form",
			evaluatorName.c_str(), this->getName(fieldmlEft->getFmlNodeParametersArgument()).c_str());
		FieldmlEft::deaccess(fieldmlEft);
		return 0;
	}
	FmlObjectHandle fmlLocalToGlobalNodeMap = Fieldml_GetBindByArgument(this->fmlSession, fmlEvaluator, this->fmlNodesArgument);
	if (!this->isValidLocalToGlobalNodeMap(fmlLocalToGlobalNodeMap, fieldmlEft->getFmlLocalNodesArgument()))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader.  Mesh element evaluator %s does not bind a valid local to global node map, hence is of invalid form",
			evaluatorName.c_str());
		FieldmlEft::deaccess(fieldmlEft);
		return 0;
	}
	FmlObjectHandle fmlChartArgument = Fieldml_GetObjectByDeclaredName(this->fmlSession, libraryChartArgumentNames[this->mesh->getDimension()]);
	if (fmlChartArgument == FML_INVALID_OBJECT_HANDLE)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader.  For mesh element evaluator %s failed to get generic chart argument %s",
			evaluatorName.c_str(), libraryChartArgumentNames[this->mesh->getDimension()]);
		FieldmlEft::deaccess(fieldmlEft);
		return 0;
	}
	if (Fieldml_GetBindByArgument(this->fmlSession, fmlEvaluator, fmlChartArgument) != this->fmlMeshChartArgument)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader.  Mesh element evaluator %s does not bind the mesh chart, hence is of invalid form",
			evaluatorName.c_str());
		FieldmlEft::deaccess(fieldmlEft);
		return 0;
	}
	const int bindCount = Fieldml_GetBindCount(this->fmlSession, fmlEvaluator);
	if (bindCount != 3)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader.  Mesh element evaluator %s does not have exactly 3 binds, hence is of invalid form",
			evaluatorName.c_str(), this->getName(fieldmlEft->getFmlNodeParametersArgument()).c_str());
		FieldmlEft::deaccess(fieldmlEft);
		return 0;
	}

	MeshElementEvaluator *meshElementEvaluator = new MeshElementEvaluator(fieldmlEft);
	FieldmlEft::deaccess(fieldmlEft);
	if (meshElementEvaluator)
	{
		meshElementEvaluator->setFmlLocalToGlobalNodeMap(fmlLocalToGlobalNodeMap);
	}
	else
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::readMeshElementEvaluatorEft.  Failed to create mesh element evaluator for %s",
			evaluatorName.c_str());
	}
	return meshElementEvaluator;
}

MeshElementEvaluator* FieldMLReader::readMeshElementEvaluator(FmlObjectHandle fmlEvaluator)
{
	const std::string evaluatorName = this->getName(fmlEvaluator);
	if ((FHT_REFERENCE_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlEvaluator))
		|| (!this->isValueTypeReal1D(fmlEvaluator)))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Mesh element evaluator %s is not a real-valued reference evaluator",
			evaluatorName.c_str());
		return 0;
	}
	const int bindCount = Fieldml_GetBindCount(this->fmlSession, fmlEvaluator);
	MeshElementEvaluator *meshElementEvaluator = (bindCount == 2) ?
		this->readMeshElementEvaluatorLegacy(fmlEvaluator) :
		this->readMeshElementEvaluatorEft(fmlEvaluator);
	if (!meshElementEvaluator)
		return 0; // error already reported

	// now read the local-to-global node map
	FE_mesh_element_field_template_data *meshEftData = this->mesh->getElementfieldtemplateData(meshElementEvaluator->getEft()); // non-accessed
	if (!meshEftData)
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::readMeshElementEvaluator.  FE_mesh getElementfieldtemplateData failed");
		delete meshElementEvaluator;
		return 0;
	}
	MeshLocalToGlobalNodeParameterConsumer localToGlobalNodeConsumer(*meshEftData);
	if (!this->readParameters<int>(meshElementEvaluator->getFmlLocalToGlobalNodeMap(), localToGlobalNodeConsumer))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Failed to read local-to-global node map for mesh element evaluator %s",
			evaluatorName.c_str());
		return 0;
	}

	// Future: read scale factors

	// store in map so quick to find next time:
	this->meshElementEvaluatorMap[fmlEvaluator] = meshElementEvaluator;
	return meshElementEvaluator;
}

FE_mesh_field_template *FieldMLReader::readMeshFieldTemplate(FmlObjectHandle fmlEvaluator)
{
	// return existing if already read
	auto iter = this->meshfieldtemplateMap.find(fmlEvaluator);
	if (iter != this->meshfieldtemplateMap.end())
		return iter->second;

	const std::string evaluatorName = this->getName(fmlEvaluator);
	if ((FHT_PIECEWISE_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlEvaluator))
		|| (!this->isValueTypeReal1D(fmlEvaluator)))
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Mesh field template evaluator %s is not a real-valued piecewise evaluator",
			evaluatorName.c_str());
		return 0;
	}

	const int indexEvaluatorCount = Fieldml_GetIndexEvaluatorCount(this->fmlSession, fmlEvaluator);
	if (1 != indexEvaluatorCount)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Mesh field template evaluator %s has %d index evaluators, expected 1",
			evaluatorName.c_str(), indexEvaluatorCount);
		return 0;
	}
	const FmlObjectHandle fmlIndexEvaluator = Fieldml_GetIndexEvaluator(this->fmlSession, fmlEvaluator, 1);
	DsLabelIdentifierArray indirectElementFunctionIdMap(CMZN_BLOCK_ARRAY_DEFAULT_BLOCK_SIZE_BYTES/sizeof(DsLabelIndex), /*allocInitValueIn*/-1);
	const int bindCount = Fieldml_GetBindCount(this->fmlSession, fmlEvaluator);
	const bool indirectMap = (1 == bindCount);
	if (indirectMap)
	{
		// indirect element -> function map: a parameter mapping element to functionId
		FmlObjectHandle fmlIndirectFunctionIdMapEvaluator = Fieldml_GetBindByArgument(this->fmlSession, fmlEvaluator, fmlIndexEvaluator);
		if (FML_INVALID_OBJECT_HANDLE == fmlIndirectFunctionIdMapEvaluator)
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Expected indirect function map to be bound to index argument %s for mesh field template evaluator %s",
				this->getName(fmlIndexEvaluator).c_str(), evaluatorName.c_str());
			return 0;
		}
		DsLabelIdentifierArrayParameterConsumer labelIdentifierArrayParameterConsumer(this->mesh->getLabels(), indirectElementFunctionIdMap);
		if (!this->readParameters<int>(fmlIndirectFunctionIdMapEvaluator, labelIdentifierArrayParameterConsumer))
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Failed to read indirect function map for mesh field template evaluator %s",
				evaluatorName.c_str());
			return 0;
		}
	}
	else if (0 == bindCount)
	{
		// direct element -> function map
		if (fmlIndexEvaluator != this->fmlElementsArgument)
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Expected mesh field template evaluator %s index to be elements argument %s for direct form",
				evaluatorName.c_str(), this->getName(this->fmlElementsArgument).c_str());
			return 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Unexpected form (%d binds) for mesh field template evaluator %s",
			bindCount, evaluatorName.c_str());
		return 0;
	}

	auto mft = this->mesh->createBlankMeshFieldTemplate();
	if (!mft)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  Failed to create mesh field template for evaluator %s",
			evaluatorName.c_str());
		return 0;
	}
	// transfer ownership of mft access to FieldmlReader so cleaned up on early exit, and to be found next time
	this->meshfieldtemplateMap[fmlEvaluator] = mft;
	const int elementEvaluatorMapCount = Fieldml_GetEvaluatorCount(this->fmlSession, fmlEvaluator);
	const FmlObjectHandle fmlDefaultElementEvaluator = Fieldml_GetDefaultEvaluator(this->fmlSession, fmlEvaluator);
	FmlObjectHandle fmlElementEvaluator = fmlDefaultElementEvaluator;
	FmlObjectHandle fmlLastElementEvaluator = FML_INVALID_OBJECT_HANDLE;
	MeshElementEvaluator *meshElementEvaluator = 0;
	// iterate over elements in default order of creation ???GRC check
	const DsLabelIndex elementIndexLimit = this->mesh->getLabelsIndexSize();
	for (DsLabelIndex elementIndex = 0; elementIndex < elementIndexLimit; ++elementIndex)
	{
		const DsLabelIdentifier elementIdentifier = this->mesh->getElementIdentifier(elementIndex);
		if (DS_LABEL_IDENTIFIER_INVALID == elementIdentifier)
		{
			display_message(ERROR_MESSAGE, "FieldML Reader:  Missing element in mesh when reading mesh field template evaluator %s",
				evaluatorName.c_str());
			return 0;
		}
		if (0 < elementEvaluatorMapCount)
		{
			const DsLabelIdentifier functionId = (indirectMap) ? indirectElementFunctionIdMap.getValue(elementIndex) : elementIdentifier;
			if (DS_LABEL_IDENTIFIER_INVALID != functionId)
				fmlElementEvaluator = Fieldml_GetElementEvaluator(this->fmlSession, fmlEvaluator, static_cast<int>(functionId), /*allowDefault*/1);
			else
				fmlElementEvaluator = fmlDefaultElementEvaluator;
		}
		if (FML_INVALID_OBJECT_HANDLE == fmlElementEvaluator)
			continue; // not defined on this element
		if (fmlElementEvaluator != fmlLastElementEvaluator)
		{
			meshElementEvaluator = this->getMeshElementEvaluator(fmlElementEvaluator);
			if (!meshElementEvaluator)
			{
				meshElementEvaluator = this->readMeshElementEvaluator(fmlElementEvaluator);
				if (!meshElementEvaluator)
				{
					display_message(ERROR_MESSAGE, "FieldML Reader:  Failed to read element evaluator for element %d in mesh field template evaluator %s",
						elementIdentifier, evaluatorName.c_str());
					return 0;
				}
			}
			fmlLastElementEvaluator = fmlElementEvaluator;
		}
		if (!mft->setElementfieldtemplate(elementIndex, meshElementEvaluator->getEft()))
		{
			display_message(ERROR_MESSAGE, "FieldMLReader::readMeshFieldTemplate.  Failed to set element field template for element %d in mesh field template evaluator %s",
				elementIdentifier, evaluatorName.c_str());
			return 0;
		}
	}
	return mft;
}

// @param fmlComponentType  Only provide if multi-component, otherwise pass FML_INVALID_OBJECT_HANDLE
int FieldMLReader::readField(FmlObjectHandle fmlFieldEvaluator, FmlObjectHandle fmlComponentsType,
	std::vector<FmlObjectHandle> &fmlComponentEvaluators, FmlObjectHandle fmlNodeParameters,
	FmlObjectHandle fmlNodeParametersArgument, FmlObjectHandle fmlNodesArgument,
	FmlObjectHandle fmlElementArgument)
{
	int return_code = 1;
	const int componentCount = static_cast<int>(fmlComponentEvaluators.size());

	std::string fieldName = this->getName(fmlFieldEvaluator);

	if (verbose)
	{
		display_message(INFORMATION_MESSAGE, "\n==> Defining field from evaluator %s, %d components\n",
			fieldName.c_str(), componentCount);
	}
	if (!this->mesh)
	{
		display_message(ERROR_MESSAGE, "FieldML Reader:  No current mesh; can't read field %s",
			fieldName.c_str(), getName(fmlNodeParameters).c_str());
		return_code = 0;
	}

	cmzn_field_id field = cmzn_fieldmodule_create_field_finite_element(field_module, componentCount);
	FE_field *feField = 0;
	Computed_field_get_type_finite_element(field, &feField);
	cmzn_field_set_name(field, fieldName.c_str());
	cmzn_field_set_managed(field, true);
	if ((componentCount >= this->mesh->getDimension()) && (componentCount <= 3))
	{
		// if field value type is RC coordinates, set field 'type coordinate' flag.
		// Needed to define faces, and by cmgui to find default coordinate field.
		FmlObjectHandle fmlValueType = Fieldml_GetValueType(fmlSession, fmlFieldEvaluator);
		std::string valueTypeName = this->getDeclaredName(fmlValueType);
		if ((valueTypeName == "coordinates.rc.3d") ||
			(valueTypeName == "coordinates.rc.2d") ||
			(valueTypeName == "coordinates.rc.1d"))
		{
			cmzn_field_set_type_coordinate(field, true);
		}
	}

	// set node parameters (nodes have already been created)
	cmzn_nodeset_id nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(field_module, CMZN_FIELD_DOMAIN_TYPE_NODES);
	HDsMapDouble nodeParameters(this->getContinuousParameters(fmlNodeParameters));
	if (!nodeParameters)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Field %s node parameters %s could not be read.",
			fieldName.c_str(), getName(fmlNodeParameters).c_str());
		return_code = 0;
	}
	if (return_code)
	{
		HDsLabels nodesLabels(getLabelsForEnsemble(this->fmlNodesType));
		HDsLabels componentsLabels;
		int componentsIndex = -1, componentsSize = 1;
		if (FML_INVALID_OBJECT_HANDLE != fmlComponentsType)
		{
			cmzn::SetImpl(componentsLabels, this->getLabelsForEnsemble(fmlComponentsType));
			if (!(componentsLabels &&
				(0 <= (componentsIndex = nodeParameters->getLabelsIndex(*componentsLabels))) &&
				(0 < (componentsSize = componentsLabels->getSize()))))
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Field %s node parameters %s is missing components index.",
					fieldName.c_str(), getName(fmlNodeParameters).c_str());
				return_code = 0;
			}
		}
		HDsLabels nodeDerivativesLabels;
		int nodeDerivativesIndex = -1, nodeDerivativesSize = 1;
		if (FML_INVALID_OBJECT_HANDLE != this->fmlNodeDerivativesArgument)
		{
			cmzn::SetImpl(nodeDerivativesLabels, this->getLabelsForEnsemble(this->fmlNodeDerivativesType));
			if (nodeDerivativesLabels)
			{
				nodeDerivativesIndex = nodeParameters->getLabelsIndex(*nodeDerivativesLabels);
				if (0 <= nodeDerivativesIndex)
					nodeDerivativesSize = nodeDerivativesLabels->getSize();
			}
			else
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Missing node derivatives labels");
				return_code = 0;
			}
		}
		HDsLabels nodeVersionsLabels;
		int nodeVersionsIndex = -1, nodeVersionsSize = 1;
		if (FML_INVALID_OBJECT_HANDLE != this->fmlNodeVersionsArgument)
		{
			cmzn::SetImpl(nodeVersionsLabels, this->getLabelsForEnsemble(this->fmlNodeVersionsType));
			if (nodeVersionsLabels)
			{
				nodeVersionsIndex = nodeParameters->getLabelsIndex(*nodeVersionsLabels);
				if (0 <= nodeVersionsIndex)
					nodeVersionsSize = nodeVersionsLabels->getSize();
			}
			else
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Missing node versions labels");
				return_code = 0;
			}
		}
		const int valuesSize = componentsSize*nodeDerivativesSize*nodeVersionsSize;
		int componentsOffset = 1;
		int nodeDerivativesOffset = 1;
		int nodeVersionsOffset = 1;
		int index[3] = { componentsIndex, nodeDerivativesIndex, nodeVersionsIndex };
		int size[3] = { componentCount, nodeDerivativesSize, nodeVersionsSize };
		int *offset[3] = { &componentsOffset, &nodeDerivativesOffset, &nodeVersionsOffset };
		// get offsets for components, derivatives and versions parameters in any order:
		int highestIndex = 0;
		for (int j = 0; j < 3; ++j)
			if (index[j] > highestIndex)
				highestIndex = index[j];
		for (int i = 0; i <= highestIndex; ++i)
			for (int j = 0; j < 3; ++j)
				if (index[j] == i)
				{
					for (int k = 0; k < 3; ++k)
						if (index[k] > i)
							*offset[j] *= size[k];
					break;
				}
		double *values = new double[valuesSize];
		// store both current and last value exists in same array and swap pointers
		int *allocatedValueExists = new int[2*valuesSize];
		int *valueExists = allocatedValueExists;
		valueExists[0] = 0;
		int *lastValueExists = allocatedValueExists + valuesSize;
		lastValueExists[0] = -5;
		cmzn_nodetemplate_id nodetemplate = 0;
		HDsMapIndexing nodeParametersIndexing(nodeParameters->createIndexing());
		DsLabelIterator *nodesLabelIterator = nodesLabels->createLabelIterator();
		FE_nodeset *feNodeset = cmzn_nodeset_get_FE_nodeset_internal(nodeset);
		if (!nodesLabelIterator)
			return_code = CMZN_ERROR_MEMORY;
		else
		{
			while (nodesLabelIterator->increment())
			{
				nodeParametersIndexing->setEntry(*nodesLabelIterator);
				int valuesRead = 0;
				if (nodeParameters->getValuesSparse(*nodeParametersIndexing, valuesSize, values, valueExists, valuesRead))
				{
					if (0 < valuesRead)
					{
						// try to reuse template if you can
						for (int i = 0; i < valuesSize; ++i)
						{
							if (valueExists[i] != lastValueExists[i])
							{
								cmzn_nodetemplate_destroy(&nodetemplate);
								nodetemplate = cmzn_nodeset_create_nodetemplate(nodeset);
								cmzn_nodetemplate_define_field(nodetemplate, field);
								// don't want VALUE by default
								cmzn_nodetemplate_set_value_number_of_versions(nodetemplate, field, /*component=all*/-1,
									CMZN_NODE_VALUE_LABEL_VALUE, /*versionsCount*/0);
								for (int c = 0; c < componentCount; ++c)
								{
									int *exists = valueExists + c*componentsOffset;
									for (int d = 0; d < nodeDerivativesSize; ++d)
									{
										for (int v = nodeVersionsSize - 1; 0 <= v; --v)
											if (exists[v*nodeVersionsOffset])
											{
												cmzn_nodetemplate_set_value_number_of_versions(nodetemplate, field, c + 1,
													static_cast<cmzn_node_value_label>(d + CMZN_NODE_VALUE_LABEL_VALUE), v + 1);
												break;
											}
										exists += nodeDerivativesOffset;
									}
								}
								break;
							}
						}
						const int nodeIdentifier = nodesLabelIterator->getIdentifier();
						cmzn_node *node = feNodeset->findNodeByIdentifier(nodeIdentifier);
						if (!cmzn_node_merge(node, nodetemplate) ||
							(CMZN_OK != FE_field_assign_node_parameters_sparse_FE_value(feField, node,
								valuesSize, values, valueExists, valuesRead, componentCount, componentsOffset,
								nodeDerivativesSize, nodeDerivativesOffset, nodeVersionsSize, nodeVersionsOffset)))
						{
							display_message(ERROR_MESSAGE, "Read FieldML:  Failed to set field %s parameters at node %d",
								fieldName.c_str(), nodeIdentifier);
							return_code = 0;
							break;
						}
						int *tmp = lastValueExists;
						lastValueExists = valueExists;
						valueExists = tmp;
					}
				}
				else
				{
					display_message(ERROR_MESSAGE, "Read FieldML:  Failed to get field %s node parameters at node %d",
						fieldName.c_str(), nodesLabelIterator->getIdentifier());
					return_code = 0;
					break;
				}
			}
		}
		cmzn::Deaccess(nodesLabelIterator);
		delete[] allocatedValueExists;
		delete[] values;
		cmzn_nodetemplate_destroy(&nodetemplate);
	}
	cmzn_nodeset_destroy(&nodeset);

	// define element fields

	FE_mesh_field_data *meshFieldData = 0;
	if (return_code)
	{
		meshFieldData = feField->getMeshFieldData(this->mesh);
		if (!meshFieldData)
		{
			meshFieldData = feField->createMeshFieldData(this->mesh);
			if (!meshFieldData)
			{
				display_message(ERROR_MESSAGE, "FieldMLReader::readField.  Failed to create mesh field data");
				return_code = 0;
			}
		}
	}
	if (return_code)
	{
		for (int ic = 0; ic < componentCount; ic++)
		{
			FE_mesh_field_template *mft = this->readMeshFieldTemplate(fmlComponentEvaluators[ic]);
			if (!mft)
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Failed to get mesh field template for field %s component %d",
					fieldName.c_str(), ic + 1);
				return_code = 0;
				break;
			}
			// GRC check; should this return bool?
			meshFieldData->setComponentMeshfieldtemplate(ic, mft);
		}
	}
	cmzn_field_destroy(&field);

	return return_code;
}

/**
 * Test whether the evaluator is scalar, continuous and piecewise over elements
 * of the mesh, directly or indirectly via a map to an intermediate ensemble,
 * and a function of the same element argument evaluator.
 * @param fmlEvaluator  The evaluator to check.
 * @param fmlElementArgument  On true result, set to the element argument the
 * piecewise evaluator is ultimately indexed over.
 * @return  Boolean true if in recognised form, false if not.
 */
bool FieldMLReader::evaluatorIsScalarContinuousPiecewiseOverElements(FmlObjectHandle fmlEvaluator,
	FmlObjectHandle &fmlElementArgument)
{
	if (FHT_PIECEWISE_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlEvaluator))
		return false;
	FmlObjectHandle fmlValueType = Fieldml_GetValueType(this->fmlSession, fmlEvaluator);
	if (FHT_CONTINUOUS_TYPE != Fieldml_GetObjectType(this->fmlSession, fmlValueType))
		return false;
	if (1 != Fieldml_GetTypeComponentCount(this->fmlSession, fmlValueType))
		return false;
	FmlObjectHandle fmlPiecewiseIndex = Fieldml_GetIndexEvaluator(this->fmlSession, fmlEvaluator, /*evaluatorIndex*/1);
	if (fmlPiecewiseIndex == FML_INVALID_OBJECT_HANDLE)
	{
		display_message(ERROR_MESSAGE, "Read FieldML:  Piecewise Evaluator %s has no index evaluator",
			this->getName(fmlEvaluator).c_str());
		return false;
	}
	// can either be directly indexed by elements, or indirectly by map
	// from elements to intermediate ensemble (i.e. a function ID)
	int bindCount = Fieldml_GetBindCount(fmlSession, fmlEvaluator);
	if (1 == bindCount)
	{
		// test for recognised indirect form: a parameter mapping element to piecewise index
		FmlObjectHandle fmlBindArgument = Fieldml_GetBindArgument(fmlSession, fmlEvaluator, 1);
		FmlObjectHandle fmlBindEvaluator = Fieldml_GetBindEvaluator(fmlSession, fmlEvaluator, 1);
		if (fmlBindArgument != fmlPiecewiseIndex)
			return false;
		if (FHT_PARAMETER_EVALUATOR != Fieldml_GetObjectType(fmlSession, fmlBindEvaluator))
			return false;
		int parameterIndexCount = Fieldml_GetIndexEvaluatorCount(fmlSession, fmlBindEvaluator);
		if (1 != parameterIndexCount)
			return false;
		fmlPiecewiseIndex = Fieldml_GetIndexEvaluator(this->fmlSession, fmlBindEvaluator, /*evaluatorIndex*/1);
		if (fmlPiecewiseIndex == FML_INVALID_OBJECT_HANDLE)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Parameter Evaluator %s has no index evaluator",
				this->getName(fmlBindEvaluator).c_str());
			return false;
		}
	}
	else if (1 < bindCount)
		return false;
	FmlObjectHandle fmlIndexType = Fieldml_GetValueType(this->fmlSession, fmlPiecewiseIndex);
	if (fmlIndexType != this->fmlElementsType)
		return false;
	fmlElementArgument = fmlPiecewiseIndex;
	return true;
}

/**
 * @param fmlComponentsArgument  Optional, for multi-component fields only.
 */
bool FieldMLReader::evaluatorIsNodeParameters(FmlObjectHandle fmlNodeParameters,
	FmlObjectHandle fmlComponentsArgument)
{
	if ((Fieldml_GetObjectType(this->fmlSession, fmlNodeParameters) != FHT_PARAMETER_EVALUATOR) ||
		(Fieldml_GetObjectType(this->fmlSession, Fieldml_GetValueType(fmlSession, fmlNodeParameters)) != FHT_CONTINUOUS_TYPE))
	{
		if (verbose)
			display_message(WARNING_MESSAGE, "Read FieldML:  %s is not continuous parameters type so can't be node parameters",
				this->getName(fmlNodeParameters).c_str());
		return false;
	}
	// check arguments are nodes, components (if any) and optionally derivatives, versions
	FmlObjectHandle fmlPossibleNodesArgument = FML_INVALID_OBJECT_HANDLE;
	bool hasComponents = false;
	bool hasDerivatives = false;
	bool hasVersions = false;
	int usedIndexCount = 0;
	int nodeParametersIndexCount = Fieldml_GetIndexEvaluatorCount(this->fmlSession, fmlNodeParameters);
	for (int i = 1; i <= nodeParametersIndexCount; ++i)
	{
		FmlObjectHandle fmlIndexEvaluator = Fieldml_GetIndexEvaluator(fmlSession, fmlNodeParameters, i);
		if (FML_INVALID_OBJECT_HANDLE == fmlIndexEvaluator)
		{
			display_message(WARNING_MESSAGE, "Read FieldML:  %s is missing index %d", this->getName(fmlNodeParameters).c_str(), i);
			return false;
		}
		if (fmlIndexEvaluator == fmlComponentsArgument)
		{
			if (hasComponents)
			{
				display_message(WARNING_MESSAGE, "Read FieldML:  %s binds to components more than once", this->getName(fmlNodeParameters).c_str());
				return false;
			}
			hasComponents = true;
			++usedIndexCount;
		}
		else if (fmlIndexEvaluator == this->fmlNodeDerivativesArgument)
		{
			if (hasDerivatives)
			{
				display_message(WARNING_MESSAGE, "Read FieldML:  %s binds to derivatives more than once", this->getName(fmlNodeParameters).c_str());
				return false;
			}
			hasDerivatives = true;
			++usedIndexCount;
		}
		else if (fmlIndexEvaluator == this->fmlNodeVersionsArgument)
		{
			if (hasVersions)
			{
				display_message(WARNING_MESSAGE, "Read FieldML:  %s binds to versions more than once", this->getName(fmlNodeParameters).c_str());
				return false;
			}
			hasVersions = true;
			++usedIndexCount;
		}
		else if (FML_INVALID_OBJECT_HANDLE == fmlPossibleNodesArgument)
		{
			fmlPossibleNodesArgument = fmlIndexEvaluator;
			++usedIndexCount;
		}
	}
	if (usedIndexCount != nodeParametersIndexCount)
	{
		display_message(WARNING_MESSAGE, "Read FieldML:  %s has unexpected extra index evaluators", this->getName(fmlNodeParameters).c_str());
		return false;
	}
	if ((fmlComponentsArgument != FML_INVALID_OBJECT_HANDLE) && !hasComponents)
	{
		display_message(WARNING_MESSAGE, "Read FieldML:  %s has unexpected extra index evaluators", this->getName(fmlNodeParameters).c_str());
		return false;
	}
	if (FML_INVALID_OBJECT_HANDLE == fmlPossibleNodesArgument)
	{
		display_message(WARNING_MESSAGE, "Read FieldML:  %s is not indexed by nodes", this->getName(fmlNodeParameters).c_str());
		return false;
	}
	if (fmlPossibleNodesArgument != this->fmlNodesArgument)
	{
		if (this->fmlNodesArgument == FML_INVALID_OBJECT_HANDLE)
		{
			if (CMZN_OK != this->readNodes(fmlPossibleNodesArgument))
				return false;
		}
		else
		{
			display_message(WARNING_MESSAGE, "Read FieldML:  %s is not indexed by standard nodes argument", this->getName(fmlNodeParameters).c_str());
			return false;
		}
	}
	return true;
}

/** continuous-valued aggregates of piecewise varying with mesh elements are
 * interpreted as vector-valued finite element fields */
int FieldMLReader::readAggregateFields()
{
	const int aggregateCount = Fieldml_GetObjectCount(this->fmlSession, FHT_AGGREGATE_EVALUATOR);
	int return_code = 1;
	for (int aggregateIndex = 1; (aggregateIndex <= aggregateCount) && return_code; aggregateIndex++)
	{
		FmlObjectHandle fmlAggregate = Fieldml_GetObject(this->fmlSession, FHT_AGGREGATE_EVALUATOR, aggregateIndex);
		std::string fieldName = getName(fmlAggregate);

		//int indexCount = Fieldml_GetIndexEvaluatorCount(this->fmlSession, fmlAggregate);
		FmlObjectHandle fmlComponentsArgument = Fieldml_GetIndexEvaluator(this->fmlSession, fmlAggregate, 1);
		if (FML_INVALID_OBJECT_HANDLE == fmlComponentsArgument)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Aggregate %s has missing index. Skipping.", fieldName.c_str());
			continue;
		}
		if (FHT_ARGUMENT_EVALUATOR != Fieldml_GetObjectType(this->fmlSession, fmlComponentsArgument))
		{
			if (this->verbose)
				display_message(WARNING_MESSAGE, "Read FieldML:  Aggregate %s index is not an argument evaluator. Skipping.", fieldName.c_str());
			continue;
		}
		FmlObjectHandle fmlValueType = Fieldml_GetValueType(this->fmlSession, fmlAggregate);
		if (FHT_CONTINUOUS_TYPE != Fieldml_GetObjectType(this->fmlSession, fmlValueType))
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Aggregate %s is not continuous type. Skipping.", fieldName.c_str());
			continue;
		}
		FmlObjectHandle fmlComponentsType = Fieldml_GetTypeComponentEnsemble(this->fmlSession, fmlValueType);
		if (Fieldml_GetValueType(this->fmlSession, fmlComponentsArgument) != fmlComponentsType)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Aggregate %s index evaluator and component types do not match. Skipping.", fieldName.c_str());
			continue;
		}
		const int componentCount = Fieldml_GetMemberCount(this->fmlSession, fmlComponentsType);

		// check components are scalar, piecewise over elements
		bool validComponents = true;
		std::vector<FmlObjectHandle> fmlComponentEvaluators(componentCount, FML_INVALID_OBJECT_HANDLE);
		FmlObjectHandle fmlElementArgument = FML_INVALID_OBJECT_HANDLE;
		for (int componentIndex = 1; componentIndex <= componentCount; componentIndex++)
		{
			// Component ensemble identifiers are always 1..N:
			int componentIdentifier = componentIndex;
			int ic = componentIndex - 1;
			fmlComponentEvaluators[ic] = Fieldml_GetElementEvaluator(fmlSession, fmlAggregate, componentIdentifier, /*allowDefault*/1);
			if (fmlComponentEvaluators[ic] == FML_INVALID_OBJECT_HANDLE)
			{
				display_message(ERROR_MESSAGE, "Read FieldML:  Aggregate %s component %d evaluator is missing", fieldName.c_str(), componentIndex);
				return_code = 0;
				break;
			}
			FmlObjectHandle fmlComponentElementArgument = FML_INVALID_OBJECT_HANDLE;
			if (!this->evaluatorIsScalarContinuousPiecewiseOverElements(
				fmlComponentEvaluators[ic], fmlComponentElementArgument))
			{
				if (verbose)
					display_message(WARNING_MESSAGE, "Read FieldML:  Aggregate %s component %d is not piecewise over elements",
						fieldName.c_str(), componentIndex);
				validComponents = false;
				break;
			}
			if (fmlElementArgument == FML_INVALID_OBJECT_HANDLE)
				fmlElementArgument = fmlComponentElementArgument;
			else if (fmlComponentElementArgument != fmlElementArgument)
			{
				if (verbose)
					display_message(ERROR_MESSAGE, "Read FieldML:  Aggregate %s components must use same element argument",
						fieldName.c_str());
				validComponents = false;
				break;
			}
		}
		if (!return_code)
			break;
		if (!validComponents)
		{
			if (verbose)
				display_message(WARNING_MESSAGE, "Read FieldML:  Aggregate %s cannot be interpreted as a field defined over a mesh. Skipping.",
					fieldName.c_str());
			continue;
		}

		// determine if exactly one binding of node parameters
		int bindCount = Fieldml_GetBindCount(fmlSession, fmlAggregate);
		if (1 != bindCount)
		{
			display_message(WARNING_MESSAGE, "Read FieldML:  Aggregate %s does not have exactly 1 binding (for node parameters). Skipping.",
				fieldName.c_str());
			continue;
		}
		FmlObjectHandle fmlNodeParametersArgument = Fieldml_GetBindArgument(fmlSession, fmlAggregate, 1);
		FmlObjectHandle fmlNodeParameters = Fieldml_GetBindEvaluator(fmlSession, fmlAggregate, 1);
		if (!this->evaluatorIsNodeParameters(fmlNodeParameters, fmlComponentsArgument))
		{
			display_message(WARNING_MESSAGE, "Read FieldML:  Aggregate %s bound argument %s is not valid node parameters. Skipping.",
				fieldName.c_str(), this->getName(fmlNodeParameters).c_str());
			continue;
		}

		return_code = readField(fmlAggregate, fmlComponentsType, fmlComponentEvaluators, fmlNodeParameters,
			fmlNodeParametersArgument, fmlNodesArgument, fmlElementArgument);
	}
	return return_code;
}

/** continuous-valued references to piecewise varying with mesh elements are
 * interpreted as scalar-valued finite element fields */
int FieldMLReader::readReferenceFields()
{
	const int referenceCount = Fieldml_GetObjectCount(fmlSession, FHT_REFERENCE_EVALUATOR);
	int return_code = 1;
	for (int referenceIndex = 1; (referenceIndex <= referenceCount) && return_code; referenceIndex++)
	{
		FmlObjectHandle fmlReference = Fieldml_GetObject(fmlSession, FHT_REFERENCE_EVALUATOR, referenceIndex);
		std::string fieldName = getName(fmlReference);

		FmlObjectHandle fmlValueType = Fieldml_GetValueType(fmlSession, fmlReference);
		if (FHT_CONTINUOUS_TYPE != Fieldml_GetObjectType(fmlSession, fmlValueType))
		{
			//display_message(WARNING_MESSAGE, "Read FieldML:  Ignore reference %s as not continuous type\n", fieldName.c_str());
			continue;
		}

		// check reference evaluator is scalar, piecewise over elements
		FmlObjectHandle fmlComponentEvaluator = Fieldml_GetReferenceSourceEvaluator(fmlSession, fmlReference);
		if (fmlComponentEvaluator == FML_INVALID_OBJECT_HANDLE)
		{
			display_message(ERROR_MESSAGE, "Read FieldML:  Reference %s source evaluator is missing", fieldName.c_str());
			return_code = 0;
			break;
		}
		FmlObjectHandle fmlElementArgument = FML_INVALID_OBJECT_HANDLE;
		if (!this->evaluatorIsScalarContinuousPiecewiseOverElements(
			fmlComponentEvaluator, fmlElementArgument))
		{
			if (verbose)
				display_message(WARNING_MESSAGE,
					"Read FieldML:  Reference %s cannot be interpreted as a field defined over a mesh. Skipping.",
					fieldName.c_str());
			continue;
		}

		// determine if exactly one binding of node parameters
		int bindCount = Fieldml_GetBindCount(fmlSession, fmlReference);
		if (1 != bindCount)
		{
			if (verbose)
			{
				display_message(WARNING_MESSAGE,
					"Read FieldML:  Reference %s does not have exactly 1 binding (for nodal parameters). Skipping.",
					fieldName.c_str());
			}
			continue;
		}
		FmlObjectHandle fmlNodeParametersArgument = Fieldml_GetBindArgument(fmlSession, fmlReference, 1);
		FmlObjectHandle fmlNodeParameters = Fieldml_GetBindEvaluator(fmlSession, fmlReference, 1);
		if (!this->evaluatorIsNodeParameters(fmlNodeParameters, /*fmlComponentsArgument*/FML_INVALID_OBJECT_HANDLE))
		{
			display_message(WARNING_MESSAGE, "Read FieldML:  Reference %s bound argument %s is not valid node parameters. Skipping.",
				fieldName.c_str(), this->getName(fmlNodeParameters).c_str());
			continue;
		}

		std::vector<FmlObjectHandle> fmlComponentEvaluators(1, fmlComponentEvaluator);
		return_code = readField(fmlReference, /*fmlComponentsType*/FML_INVALID_OBJECT_HANDLE,
			fmlComponentEvaluators, fmlNodeParameters, fmlNodeParametersArgument, fmlNodesArgument, fmlElementArgument);
	}
	return return_code;
}

int FieldMLReader::parse()
{
	int return_code = 1;
	if ((!region) || (!filename) || (!field_module))
	{
		display_message(ERROR_MESSAGE, "FieldMLReader::parse.  Invalid construction arguments");
		return_code = 0;
	}
	if (fmlSession == (const FmlSessionHandle)FML_INVALID_HANDLE)
	{
		display_message(ERROR_MESSAGE, "Read FieldML: could not parse file %s", filename);
		return_code = 0;
	}
	else
	{
		int parseErrorCount = Fieldml_GetErrorCount(fmlSession);
		return_code = (parseErrorCount == 0);
		for (int i = 1; i <= parseErrorCount; i++)
		{
			char *error_string = Fieldml_GetError(fmlSession, i);
			display_message(ERROR_MESSAGE, "FieldML Parse error: %s", error_string);
			Fieldml_FreeString(error_string);
		}
	}
	if (!return_code)
		return 0;
	cmzn_region_begin_change(region);
	return_code = return_code && this->readGlobals();
	return_code = return_code && this->readMeshes();
	return_code = return_code && this->readAggregateFields();
	return_code = return_code && this->readReferenceFields();
	cmzn_region_end_change(region);
	return return_code;
}

bool string_contains_FieldML_tag(const char *text)
{
	return (0 != strstr(text, "<Fieldml"));
}

} // anonymous namespace

bool is_FieldML_file(const char *filename)
{
	bool result = false;
	FILE *stream = fopen(filename, "r");
	if (stream)
	{
		char block[200];
		size_t size = fread((void *)block, sizeof(char), sizeof(block), stream);
		if (size > 0)
		{
			block[size - 1] = '\0';
			result = string_contains_FieldML_tag(block);
		}
		fclose(stream);
	}
	return result;
}

bool is_FieldML_memory_block(unsigned int memory_buffer_size, const void *memory_buffer)
{
	if ((0 == memory_buffer_size) || (!memory_buffer))
		return false;
	unsigned int size = memory_buffer_size;
	if (size > 200)
		size = 200;
	char block[200];
	memcpy(block, memory_buffer, size);
	block[size - 1] = '\0';
	return string_contains_FieldML_tag(block);
}

int parse_fieldml_file(struct cmzn_region *region, const char *filename)
{
	FieldMLReader fmlReader(region, filename);
	return fmlReader.parse();
}
