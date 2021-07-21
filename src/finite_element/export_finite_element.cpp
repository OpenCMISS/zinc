/**
 * FILE : export_finite_element.cpp
 *
 * Functions for exporting finite element data to EX format.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/field.h"
#include "opencmiss/zinc/fieldgroup.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/fieldsubobjectgroup.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/node.h"
#include "opencmiss/zinc/nodeset.h"
#include "opencmiss/zinc/region.h"
#include "datastore/labels.hpp"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_private.h"
#include "finite_element/finite_element_region.h"
#include "finite_element/export_finite_element.h"
#include "general/compare.h"
#include "general/debug.h"
#include "general/enumerator_private.hpp"
#include "general/list.h"
#include "general/indexed_list_private.h"
#include "general/mystring.h"
#include "general/object.h"
#include "region/cmiss_region_write_info.h"
#include "general/message.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <map>
#include <vector>
using namespace std;

/*
Module types
------------
*/

namespace {

/** Stores offset into packed nodes for each EFT, allowing nodes for several EFTs to
  * be output together iff they share the same nodes in the element
  */
class ElementNodePacking
{
	class EftNodes
	{
	public:
		int offset;  // actually constant, but can't make const to use default assignment and copy constructor
		int count;  // actually constant, but can't make const to use default assignment and copy constructor
		std::vector<const FE_element_field_template *> efts;
		const DsLabelIndex *nodeIndexes;  // can be NULL if no eft nodes set yet

		EftNodes(int offsetIn, const FE_element_field_template *eft, const DsLabelIndex *nodeIndexesIn) :
			offset(offsetIn),
			count(eft->getNumberOfLocalNodes()),
			nodeIndexes(nodeIndexesIn)
		{
			efts.push_back(eft);
		}
	};

	std::vector<EftNodes> eftNodes;

public:

	/** @param  nodeIndexesIn  The node indexes for eft; can be NULL if not set */
	void packEftNodes(const FE_element_field_template *eft, const DsLabelIndex *nodeIndexesIn)
	{
		const size_t eftNodesCount = this->eftNodes.size();
		int offset = 0;
		for (size_t i = 0; i < eftNodesCount; ++i)
		{
			EftNodes& thisEftNodes = this->eftNodes[i];
			if (eft->getNumberOfLocalNodes() == thisEftNodes.count)
			{
				const size_t eftCount = thisEftNodes.efts.size();
				for (size_t e = 0; e < eftCount; ++e)
				{
					if (thisEftNodes.efts[e] == eft)
						return;  // already packed this EFT's nodes
				}
				if ((nodeIndexesIn) && (thisEftNodes.nodeIndexes)
					&& (0 == memcmp(thisEftNodes.nodeIndexes, nodeIndexesIn, sizeof(DsLabelIndex)*thisEftNodes.count)))
				{
					thisEftNodes.efts.push_back(eft);
					return;
				}
			}
			offset += thisEftNodes.count;
		}
		EftNodes newEftNodes(offset, eft, nodeIndexesIn);
		this->eftNodes.push_back(newEftNodes);
	}

	/** @return  Offset starting at 0 into packed nodes array where nodes for EFT
	  * are stored, or -1 if EFT nodes not packed. */
	int getEftNodeOffset(const FE_element_field_template *eft) const
	{
		const size_t eftNodesCount = this->eftNodes.size();
		for (size_t i = 0; i < eftNodesCount; ++i)
		{
			const EftNodes& thisEftNodes = this->eftNodes[i];
			if (eft->getNumberOfLocalNodes() == thisEftNodes.count)
			{
				const size_t eftCount = thisEftNodes.efts.size();
				for (size_t e = 0; e < eftCount; ++e)
				{
					if (thisEftNodes.efts[e] == eft)
						return thisEftNodes.offset;
				}
			}
		}
		return -1;
	}

	/** @return  True if other ElementNodePacking matches the same offsets for all the same EFTs. */
	bool matches(const ElementNodePacking& other) const
	{
		const size_t eftNodesCount = this->eftNodes.size();
		if (other.eftNodes.size() != eftNodesCount)
			return false;
		for (size_t i = 0; i < eftNodesCount; ++i)
		{
			const EftNodes& thisEftNodes = this->eftNodes[i];
			const EftNodes& otherEftNodes = other.eftNodes[i];
			const size_t eftCount = thisEftNodes.efts.size();
			if (otherEftNodes.efts.size() != eftCount)
				return false;
			for (size_t e = 0; e < eftCount; ++e)
			{
				if (otherEftNodes.efts[e] != thisEftNodes.efts[e])
					return false;
			}
		}
		return true;
	}

	int getTotalNodeCount() const
	{
		int totalNodeCount = 0;
		const size_t eftNodesCount = this->eftNodes.size();
		for (size_t i = 0; i < eftNodesCount; ++i)
		{
			const EftNodes& thisEftNodes = this->eftNodes[i];
			totalNodeCount += thisEftNodes.count;
		}
		return totalNodeCount;
	}

	/** @return  First EFT for node block index; iterate from 0 until no EFT returned */
	const FE_element_field_template *getFirstEftAtIndex(size_t index) const
	{
		if (index < this->eftNodes.size())
			return this->eftNodes[index].efts[0];
		return 0;
	}
};

/** Add field to vector, but ensure indexer fields added before any fields they index */
int FE_field_add_to_vector_indexer_priority(struct FE_field *field, void *field_vector_void)
{
	std::vector<FE_field*> *field_vector = static_cast<std::vector<FE_field*> *>(field_vector_void);
	for (auto fieldIter = field_vector->begin(); fieldIter != field_vector->end(); ++fieldIter)
	{
		if (*fieldIter == field)
			return 1; // already present
	}
	if (INDEXED_FE_FIELD == get_FE_field_FE_field_type(field))
	{
		FE_field *indexer_field = 0;
		int number_of_indexed_values = 0;
		get_FE_field_type_indexed(field, &indexer_field, /*ignored*/&number_of_indexed_values);
		if (indexer_field)
			FE_field_add_to_vector_indexer_priority(indexer_field, field_vector_void);
	}
	field_vector->push_back(field);
	return 1;
}

} // anonymous namespace

class EXWriter
{
	ostream *output_file;
	enum FE_write_criterion write_criterion;
	struct FE_field_order_info *field_order_info;
	std::vector<FE_field*> writableFields; // fields to write, from field_order_info, otherwise field manager
	const FE_mesh *mesh;
	const FE_nodeset *nodeset;
	struct FE_region *fe_region;
	FE_value time;
	bool writeGroupOnly;
	// following cached to check whether last field header applies to subsequent elements
	std::vector<FE_field *> headerFields;
	// following caches for elements only:
	FE_element_shape *lastElementShape;
	cmzn_element *headerElement;
	ElementNodePacking *headerElementNodePacking;
	std::vector<const FE_element_field_template *> headerScalingEfts;
	// following caches for nodes only:
	cmzn_node *headerNode;

public:

	EXWriter(ostream *output_fileIn, FE_write_criterion write_criterionIn,
			FE_field_order_info *field_order_infoIn,FE_value timeIn) :
		output_file(output_fileIn),
		write_criterion(write_criterionIn),
		field_order_info(field_order_infoIn),
		mesh(0),
		nodeset(0),
		fe_region(0),
		time(timeIn),
		writeGroupOnly(false),
		lastElementShape(0),
		headerElement(0),
		headerElementNodePacking(0)
	{
		// make a list of all fields that should be written, with indexer fields before any indexed fields using them
		if (this->field_order_info)
		{
			const int number_of_fields = get_FE_field_order_info_number_of_fields(field_order_info);
			for (int f = 0; f < number_of_fields; ++f)
				FE_field_add_to_vector_indexer_priority(get_FE_field_order_info_field(field_order_info, f), (void *)&this->writableFields);
		}
		else
		{
			FE_region_for_each_FE_field(this->fe_region, FE_field_add_to_vector_indexer_priority, (void *)&this->writableFields);
		}
	}

	~EXWriter()
	{
		this->clearHeaderCache();
	}

	void clearHeaderCache()
	{
		this->headerFields.clear();
		this->lastElementShape = 0;
		this->headerElement = 0;
		delete this->headerElementNodePacking;
		this->headerElementNodePacking = 0;
		this->headerScalingEfts.clear();
		this->headerNode = 0;
	}

	/** Switch to writing elements from mesh */
	void setMesh(const FE_mesh *meshIn)
	{
		this->mesh = meshIn;
		this->fe_region = this->mesh->get_FE_region();
		this->nodeset = 0;
		this->clearHeaderCache();
	}

	/** Switch to writing nodes from nodeset */
	void setNodeset(const FE_nodeset *nodesetIn)
	{
		this->mesh = 0;
		this->nodeset = nodesetIn;
		this->fe_region = this->nodeset->get_FE_region();
		this->clearHeaderCache();
	}

	void setWriteGroupOnly()
	{
		this->writeGroupOnly = true;
	}

	bool writeElementExt(cmzn_element *element);
	bool writeNodeExt(cmzn_node *node);

	/** Output name, automatically quoting if contains special characters */
	void writeSafeName(const char *name) const
	{
		char *safeName = duplicate_string(name);
		make_valid_token(&safeName);
		(*this->output_file) << safeName;
		DEALLOCATE(safeName);
	}

private:
	bool writeElementXiValue(const FE_mesh *hostMesh, DsLabelIndex elementIndex, const FE_value *xi);
	bool writeFieldHeader(int fieldIndex, struct FE_field *field);
	bool writeFieldValues(struct FE_field *field);
	bool writeOptionalFieldValues();
	bool writeElementShape(FE_element_shape *elementShape);
	bool writeBasis(FE_basis *basis);
	bool elementIsToBeWritten(cmzn_element *element);
	bool elementHasFieldsToWrite(cmzn_element *element);
	bool elementFieldsMatchLastElement(cmzn_element *element);
	bool writeEftScaleFactorIdentifiers(const FE_element_field_template *eft);
	bool writeElementHeaderField(cmzn_element *element, int fieldIndex, FE_field *field);
	ElementNodePacking *createElementNodePacking(cmzn_element *element);
	bool writeElementHeader(cmzn_element *element);
	bool writeElementFieldComponentValues(cmzn_element *element, FE_field *field, int componentNumber);
	bool writeElement(cmzn_element *element);

	bool nodeIsToBeWritten(cmzn_node *node);
	bool writeNodeHeaderField(cmzn_node *node, int fieldIndex, FE_field *field);
	bool writeNodeHeader(cmzn_node *node);
	bool writeNodeFieldValues(cmzn_node *node, FE_field *field);
	bool writeNode(cmzn_node *node);

};

/*
Module functions
----------------
*/

/**
 * Writes to output_file the element_xi position in the format:
 * ELEMENT_IDENTIFIER xi1 xi2... xi(DIMENSION)
 * If there is no element it writes:
 * -1 0 0... xi(DIMENSION)
 * This new format requires embedded locations be within one mesh set
 * with the element:xi field.
 */
bool EXWriter::writeElementXiValue(const FE_mesh *hostMesh, DsLabelIndex elementIndex, const FE_value *xi)
{
	if (!((hostMesh) && (xi)))
	{
		display_message(ERROR_MESSAGE, "EXWriter::writeElementXiValue.  Invalid argument(s)");
		return false;
	}
	DsLabelIdentifier elementIdentifier = hostMesh->getElementIdentifier(elementIndex);
	(*this->output_file) << " " << elementIdentifier;
	for (int d = 0; d < hostMesh->getDimension(); ++d)
	{
		if (elementIdentifier < 0)
		{
			(*this->output_file) << " 0";
		}
		else
		{
			char num_string[100];
			sprintf(num_string, " %" FE_VALUE_STRING, xi[d]);
			(*this->output_file) << num_string;
		}
	}
	return true;
}

/**
 * Writes the part of the field header that is common for nodes and elements.
 * Examples:
 * 1) coordinates, coordinate, rectangular cartesian, #Components=3
 * 2) variable, field, indexed, Index_field=bob, #Values=3, real, #Components=1
 * 3) fixed, field, constant, integer, #Components=3
 * 4) an_array, field, real, #Values=10, #Components=1
 * Value_type ELEMENT_XI_VALUE has optional Mesh Dimension=#.
 */
bool EXWriter::writeFieldHeader(int fieldIndex, struct FE_field *field)
{
	(*this->output_file) << fieldIndex << ") " << get_FE_field_name(field);
	(*this->output_file) << ", " << ENUMERATOR_STRING(CM_field_type)(field->get_CM_field_type());
	/* optional constant/indexed, Index_field=~, #Values=# */
	FE_field_type fe_field_type = get_FE_field_FE_field_type(field);
	switch (fe_field_type)
	{
	case CONSTANT_FE_FIELD:
	{
		(*this->output_file) << ", constant";
	} break;
	case GENERAL_FE_FIELD:
	{
		/* default; nothing to write */
	} break;
	case INDEXED_FE_FIELD:
	{
		struct FE_field *indexer_field;
		int number_of_indexed_values;
		if (!get_FE_field_type_indexed(field, &indexer_field, &number_of_indexed_values))
		{
			display_message(ERROR_MESSAGE, "EXWriter::writeFieldHeader.  Invalid indexed field");
			return false;
		}
		(*this->output_file) <<  ", indexed, Index_field=" << get_FE_field_name(indexer_field) << ", #Values=" << number_of_indexed_values;
	} break;
	default:
	{
		display_message(ERROR_MESSAGE, "EXWriter::writeFieldHeader.  Invalid FE_field_type");
		return false;
	} break;
	}
	const Coordinate_system& coordinate_system = field->getCoordinateSystem();
	switch (coordinate_system.type)
	{
		case CYLINDRICAL_POLAR:
		{
			(*this->output_file) << ", cylindrical polar";
		} break;
		case FIBRE:
		{
			(*this->output_file) << ", fibre";
		} break;
		case OBLATE_SPHEROIDAL:
		{
			char num_string[100];
			sprintf(num_string, "%" FE_VALUE_STRING, coordinate_system.parameters.focus);
			(*this->output_file) << ", oblate spheroidal, focus=" << num_string;
		} break;
		case PROLATE_SPHEROIDAL:
		{
			char num_string[100];
			sprintf(num_string, "%" FE_VALUE_STRING, coordinate_system.parameters.focus);
			(*this->output_file) << ", prolate spheroidal, focus=" << num_string;
		} break;
		case RECTANGULAR_CARTESIAN:
		{
			(*this->output_file) << ", rectangular cartesian";
		} break;
		case SPHERICAL_POLAR:
		{
			(*this->output_file) << ", spherical polar";
		} break;
		case NOT_APPLICABLE:
		{
			/* write nothing */
		} break;
		default:
		{
			display_message(WARNING_MESSAGE,
				"EXWriter::writeFieldHeader.  Unknown coordinate system type: ignoring");
			/* write nothing */
		} break;
	}

	// In EX Versions < 2, value type was optional if coordinate system output for field
	// Since reader always handled it if present or not, now write it always
	Value_type valueType = get_FE_field_value_type(field);
	(*this->output_file) << ", " << Value_type_string(valueType);

	const int componentCount = get_FE_field_number_of_components(field);
	(*this->output_file) << ", #Components=" << componentCount;
	if (ELEMENT_XI_VALUE == valueType)
	{
		const FE_mesh *hostMesh = field->getElementXiHostMesh();
		if (!hostMesh)
		{
			display_message(ERROR_MESSAGE, "EXWriter::writeFieldHeader.  Missing host mesh for element xi field");
			return false;
		}
		char *hostMeshName = duplicate_string(hostMesh->getName());
		make_valid_token(&hostMeshName);
		(*this->output_file) << ", host mesh=" << hostMeshName << ", host mesh dimension=" << hostMesh->getDimension();
		DEALLOCATE(hostMeshName);
	}
	(*this->output_file) << "\n";
	return true;
}

/**
  * Writes the field values to <output_file> - if field is of type constant or
  * indexed. Each component or version starts on a new line.
  */
bool EXWriter::writeFieldValues(struct FE_field *field)
{
	if (!(output_file && field))
	{
		display_message(ERROR_MESSAGE, "EXWriter::writeFieldValues.  Invalid argument(s)");
		return false;
	}
	
	const int number_of_values = field->getNumberOfValues();
	/* only output values for fields with them; ie. not for GENERAL_FE_FIELD */
	if (0<number_of_values)
	{
		Value_type valueType = get_FE_field_value_type(field);
		switch (valueType)
		{
			case FE_VALUE_VALUE:
			{
				char num_string[100];
				const FE_value *fieldValues = field->getRealValues();
				if (!fieldValues)
				{
					display_message(ERROR_MESSAGE, "EXWriter::writeFieldValues.  Error getting field FE_values");
					return false;
				}
				for (int k=0;k<number_of_values;k++)
				{
					sprintf(num_string, "%" FE_VALUE_STRING, fieldValues[k]);
					(*this->output_file) << " " << num_string;
				}
			} break;
			case INT_VALUE:
			{
				const int *fieldValues = field->getIntValues();
				if (!fieldValues)
				{
					display_message(ERROR_MESSAGE, "EXWriter::writeFieldValues.  Error getting field ints");
					return false;
				}
				for (int k=0;k<number_of_values;k++)
				{
					(*this->output_file) << " " << fieldValues[k];
				}
			} break;
			case STRING_VALUE:
			{
				const char **fieldValues = field->getStringValues();
				if (!fieldValues)
				{
					display_message(ERROR_MESSAGE, "EXWriter::writeFieldValues.  Could not get string values");
					return false;
				}
				for (int k=0;k<number_of_values;k++)
				{
					if (fieldValues[k])
					{
						char *s = duplicate_string(fieldValues[k]);
						make_valid_token(&s);
						(*this->output_file) << " " << s;
						DEALLOCATE(s);
					}
					else
					{
						/* output empty string */
						(*this->output_file) << " \"\"";
					}
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE, "EXWriter::writeFieldValues.  Value type %s not supported",
					Value_type_string(valueType));
				return false;
			} break;
		}
		(*this->output_file) << "\n";
	}
	return true;
}

/** If any fields have per-field parameters, e.g. is constant or indexed, writes a Values section
  * future: change to support having only some components constant; probably remove indexed */
bool EXWriter::writeOptionalFieldValues()
{
	if (!output_file)
	{
		display_message(ERROR_MESSAGE, "EXWriter::writeOptionalFieldValues.  Invalid argument(s)");
		return false;
	}
	bool first = true;
	for (auto fieldIter = this->headerFields.begin(); fieldIter != this->headerFields.end(); ++fieldIter)
	{
		FE_field *field = *fieldIter;
		const FE_field_type feFieldType = get_FE_field_FE_field_type(field);
		if ((feFieldType == CONSTANT_FE_FIELD) || (feFieldType == INDEXED_FE_FIELD))
		{
			if (first)
			{
				(*this->output_file) << "Values:\n";
				first = false;
			}
			if (!this->writeFieldValues(field))
				return false;
		}
	}
	return true;
}

/**
 * Writes out the element shape to stream.
 */
bool EXWriter::writeElementShape(FE_element_shape *elementShape)
{
	if (!elementShape)
	{
		display_message(ERROR_MESSAGE, "EXWriter::writeElementShape.  Invalid argument(s)");
		return false;
	}
	char *shape_description = FE_element_shape_get_EX_description(elementShape);
	if (!shape_description)
	{
		display_message(ERROR_MESSAGE, "EXWriter::writeElementShape.  Invalid shape");
		return false;
	}
	const int dimension = get_FE_element_shape_dimension(elementShape);
	(*this->output_file) << "Shape. Dimension=" << dimension << ", " << shape_description << "\n";
	DEALLOCATE(shape_description);
	this->lastElementShape = elementShape;
	return true;
}

/** Writes out the <basis> to <output_file>. */
bool EXWriter::writeBasis(FE_basis *basis)
{
	char *basis_string = FE_basis_get_description_string(basis);
	if (!basis_string)
	{
		display_message(ERROR_MESSAGE, "EXWriter::writeBasis.  Invalid basis");
		return false;
	}
	(*this->output_file) << basis_string;
	DEALLOCATE(basis_string);
	return true;
}

/** Write template defining field on element */
bool EXWriter::writeElementHeaderField(cmzn_element *element, int fieldIndex, FE_field *field)
{
	if (!this->writeFieldHeader(fieldIndex, field))
		return false;
	const FE_mesh_field_data *meshFieldData = field->getMeshFieldData(this->mesh);
	if (!meshFieldData)
		return false;

	FE_field_type fe_field_type = get_FE_field_FE_field_type(field);
	const int componentCount = get_FE_field_number_of_components(field);
	for (int c = 0; c < componentCount; ++c)
	{
		char *componentName = get_FE_field_component_name(field, c);
		if (componentName)
		{
			(*this->output_file) << " " << componentName << ". ";
			DEALLOCATE(componentName);
		}
		else
		{
			(*this->output_file) << " " << c + 1 << ".";
		}
		if (fe_field_type != GENERAL_FE_FIELD)
		{
			// constant and indexed fields: no further component information
			(*this->output_file) << "\n";
			continue;
		}

		const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
		const FE_element_field_template *eft = mft->getElementfieldtemplate(element->getIndex());
		if (!eft)
			return false;
		FE_basis *basis = eft->getBasis();
		this->writeBasis(basis);

		const FE_basis_modify_theta_mode modifyThetaMode = eft->getLegacyModifyThetaMode();
		switch (modifyThetaMode)
		{
		case FE_BASIS_MODIFY_THETA_MODE_INVALID:
			(*this->output_file) << ", no modify";
			break;
		case FE_BASIS_MODIFY_THETA_MODE_CLOSEST_IN_XI1:
			(*this->output_file) << ", closest in xi1";
			break;
		case FE_BASIS_MODIFY_THETA_MODE_DECREASING_IN_XI1:
			(*this->output_file) << ", decreasing in xi1";
			break;
		case FE_BASIS_MODIFY_THETA_MODE_INCREASING_IN_XI1:
			(*this->output_file) << ", increasing in xi1";
			break;
		case FE_BASIS_MODIFY_THETA_MODE_NON_DECREASING_IN_XI1:
			(*this->output_file) << ", non-decreasing in xi1";
			break;
		case FE_BASIS_MODIFY_THETA_MODE_NON_INCREASING_IN_XI1:
			(*this->output_file) << ", non-increasing in xi1";
			break;
		}

		const cmzn_elementfieldtemplate_parameter_mapping_mode parameterMappingMode = eft->getParameterMappingMode();
		switch (parameterMappingMode)
		{
		case CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_ELEMENT:
		{
			const int *gridNumberInXi = eft->getLegacyGridNumberInXi();
			if (0 == gridNumberInXi)
			{
				(*this->output_file) << ", element based.\n";
			}
			else
			{
				(*this->output_file) << ", grid based.\n";
				// Future: force output in old EX format
				//const int unitGridNumberInXi[MAXIMUM_ELEMENT_XI_DIMENSIONS] = { 1, 1, 1 };
				//if (!gridNumberInXi)
				//	gridNumberInXi = unitGridNumberInXi;
				(*this->output_file) << " ";
				const int dimension = this->mesh->getDimension();
				for (int d = 0; d < dimension; ++d)
				{
					if (0 < d)
						(*this->output_file) << ", ";
					(*this->output_file) << "#xi" << d + 1 << "=" << gridNumberInXi[d];
				}
				(*this->output_file) << "\n";
			}
			break;
		}
		case CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_FIELD:
		{
			(*this->output_file) << ", field based.\n";
			break;
		}
		case CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE:
		{
			(*this->output_file) << ", standard node based.";
			int scaleFactorOffset = 0;
			// previously the scale factor set was identified by the basis, now it is explicitly named
			if (eft->getNumberOfLocalScaleFactors() > 0)
			{
				int scaleFactorSetIndex = 0;
				for (auto eftIter = this->headerScalingEfts.begin(); eftIter != this->headerScalingEfts.end(); ++eftIter)
				{
					++scaleFactorSetIndex;
					if (*eftIter == eft)
						break;
					scaleFactorOffset += (*eftIter)->getNumberOfLocalScaleFactors();
				}
				(*this->output_file) << " scale factor set=scaling" << scaleFactorSetIndex;
			}
			(*this->output_file) << "\n";

			const int nodeCount = eft->getNumberOfLocalNodes();
			const int packedNodeOffset = this->headerElementNodePacking->getEftNodeOffset(eft);
			(*this->output_file) << "  #Nodes=" << nodeCount << "\n";
			// previously there was an entry for each basis node with the parameters extracted from it
			// now there is a separate entry for each block of parameters mapped from
			// the same nodes with the same number of terms
			// Note local node numbers are for the element starting at packedNodeOffset, and
			// reader is expected to respect order in element when converting to EFT indexes
			const int functionCount = eft->getNumberOfFunctions();
			int f = 0;
			while (f < functionCount)
			{
				(*this->output_file) << "  ";
				const int termCount = eft->getFunctionNumberOfTerms(f);
				int valueCount = 1;
				// for compatibility with EX versions < 2 limit function values output to those
				// for current basis node, otherwise repeated nodes will be bunched together
				int f2basisNodeLimit = FE_basis_get_basis_node_function_number_limit(basis, f);
				for (int f2 = f + 1; f2 < f2basisNodeLimit; ++f2)
				{
					if (eft->getFunctionNumberOfTerms(f2) != termCount)
						break;
					int t = 0;
					for (; t < termCount; ++t)
					{
						if (eft->getTermLocalNodeIndex(f2, t) != eft->getTermLocalNodeIndex(f, t))
							break;
					}
					if (t < termCount)
						break;
					++valueCount;
				}
				if (0 == termCount)
				{
					// Use node index 0 if no terms
					(*this->output_file) << "0";
				}
				else
				{
					for (int t = 0; t < termCount; ++t)
					{
						if (t > 0)
							(*this->output_file) << "+";
						(*this->output_file) << eft->getTermLocalNodeIndex(f, t) + 1 + packedNodeOffset;
					}
				}
				(*this->output_file) << ". #Values=" << valueCount << "\n";
				// nodal value labels(versions) e.g. d/ds1(2), or the special zero for no terms
				// multi term example: d/ds1(2)+d/ds2
				(*this->output_file) << "   Value labels:";
				const int f2limit = f + valueCount;
				for (int f2 = f; f2 < f2limit; ++f2)
				{
					(*this->output_file) << " ";
					if (termCount == 0)
					{
						(*this->output_file) << "zero";
					}
					else
					{
						for (int t = 0; t < termCount; ++t)
						{
							if (t > 0)
								(*this->output_file) << "+";
							const cmzn_node_value_label nodeValueLabel = eft->getTermNodeValueLabel(f2, t);
							const char *valueLabelName = ENUMERATOR_STRING(cmzn_node_value_label)(nodeValueLabel);
							(*this->output_file) << valueLabelName;
							const int version = eft->getTermNodeVersion(f2, t);
							if (version > 0)
								(*this->output_file) << "(" << version + 1 << ")";
						}
					}
				}
				(*this->output_file) << "\n";
				// New: scale factor indexes only output if there is any scaling for this EFT
				// For compatibility with old EX Versions, output scale factor indexes are relative
				// to the whole element template, not per EFT, so must have the appropriate offset added.
				if (eft->getNumberOfLocalScaleFactors() > 0)
				{
					(*this->output_file) << "   Scale factor indices:";
					for (int f2 = f; f2 < f2limit; ++f2)
					{
						(*this->output_file) << " ";
						if (termCount == 0)
						{
							(*this->output_file) << "0"; // 0 means unscaled, still required for zero terms
						}
						else
						{
							for (int t = 0; t < termCount; ++t)
							{
								if (t > 0)
									(*this->output_file) << "+";
								const int termScaleFactorCount = eft->getTermScalingCount(f2, t);
								if (0 == termScaleFactorCount)
								{
									(*this->output_file) << "0"; // 0 means unscaled
								}
								else
								{
									for (int s = 0; s < termScaleFactorCount; ++s)
									{
										if (s > 0)
											(*this->output_file) << "*";
										const int scaleFactorIndex = eft->getTermScaleFactorIndex(f2, t, s);
										(*this->output_file) << scaleFactorOffset + scaleFactorIndex + 1;
									}
								}
							}
						}
					}
					(*this->output_file) << "\n";
				}
				f = f2limit;
			}
		}	break;
		case CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_INVALID:
		{
			display_message(ERROR_MESSAGE, "EXWriter::writeElementHeaderField.  Invalid parameter mapping mode");
			return false;
			break;
		}
		}
	}
	return true;
}

ElementNodePacking *EXWriter::createElementNodePacking(cmzn_element *element)
{
	ElementNodePacking *elementNodePacking = new ElementNodePacking();
	const size_t fieldCount = this->writableFields.size();
	for (size_t f = 0; f < fieldCount; ++f)
	{
		FE_field *field = this->writableFields[f];
		const FE_mesh_field_data *meshFieldData = field->getMeshFieldData(this->mesh);
		if (!meshFieldData)
			continue;
		const int componentCount = get_FE_field_number_of_components(field);
		const FE_element_field_template *lastEft = 0;
		for (int c = 0; c < componentCount; ++c)
		{
			const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
			const FE_element_field_template *eft = mft->getElementfieldtemplate(element->getIndex());
			if (!eft)
				break; // field not defined on this element
			if (eft != lastEft)
			{
				if (eft->getNumberOfLocalNodes() > 0)
				{
					const FE_mesh_element_field_template_data *meshEftData = this->mesh->getElementfieldtemplateData(eft);
					const DsLabelIndex *nodeIndexes = meshEftData->getElementNodeIndexes(element->getIndex());
					if (!nodeIndexes)
					{
						display_message(WARNING_MESSAGE, "EXWriter::createElementNodePacking.  Missing node indexes");
					}
					elementNodePacking->packEftNodes(eft, nodeIndexes);
				}
				lastEft = eft;
			}
		}
	}
	return elementNodePacking;
}

/**
 * Writes EFT scale factor types and identifiers. Example:
 * global_general(11,12) node_patch(1,2,3,4)
 */
bool EXWriter::writeEftScaleFactorIdentifiers(const FE_element_field_template *eft)
{
	if (!((eft) && (eft->getNumberOfLocalScaleFactors() > 0)))
	{
		display_message(ERROR_MESSAGE, "EXWriter::writeEftScaleFactorIdentifiers.  Invalid element field template");
		return false;
	}
	cmzn_elementfieldtemplate_scale_factor_type lastScaleFactorType = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_INVALID;
	const int scaleFactorCount = eft->getNumberOfLocalScaleFactors();
	for (int s = 0; s < scaleFactorCount; ++s)
	{
		const cmzn_elementfieldtemplate_scale_factor_type scaleFactorType = eft->getScaleFactorType(s);
		const int identifier = eft->getScaleFactorIdentifier(s);
		if (scaleFactorType != lastScaleFactorType)
		{
			if (s > 0)
			{
				(*this->output_file) << ") ";
			}
			const char *typeName = 0;
			switch (scaleFactorType)
			{
			case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_GENERAL:
				typeName = "element_general";
				break;
			case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_PATCH:
				typeName = "element_patch";
				break;
			case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_GENERAL:
				typeName = "global_general";
				break;
			case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_PATCH:
				typeName = "global_patch";
				break;
			case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_GENERAL:
				typeName = "node_general";
				break;
			case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_PATCH:
				typeName = "node_patch";
				break;
			case CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_INVALID:
				break;
			}
			if (typeName == 0)
			{
				display_message(ERROR_MESSAGE, "EXWriter::writeEftScaleFactorIdentifiers.  Invalid element field template scale factor type");
				return false;
			}
			(*this->output_file) << typeName << "(" << identifier;
			lastScaleFactorType = scaleFactorType;
		}
		else
		{
			(*this->output_file) << "," << identifier;
		}
	}
	(*this->output_file) << ")";
	return true;
}

/**
 * Writes the element field information header for element. If the
 * field_order_info is supplied the header is restricted to include only
 * components/fields/bases for it.
 */
bool EXWriter::writeElementHeader(cmzn_element *element)
{
	if (!element)
	{
		display_message(ERROR_MESSAGE,
			"EXWriter::writeElementHeader.  Invalid argument(s)");
		return false;
	}

	this->headerElement = element;
	this->headerFields.clear();
	delete this->headerElementNodePacking;
	this->headerElementNodePacking = new ElementNodePacking();
	this->headerScalingEfts.clear();

	// make list of fields in header, order of EFT scale factor sets, node packing
	for (auto fieldIter = this->writableFields.begin(); fieldIter != this->writableFields.end(); ++fieldIter)
	{
		FE_field *field = *fieldIter;
		const FE_mesh_field_data *meshFieldData = field->getMeshFieldData(this->mesh);
		if (!meshFieldData)
			continue; // field not defined on mesh
		const int componentCount = get_FE_field_number_of_components(field);
		const FE_element_field_template *lastEft = 0;
		int c = 0;
		for (; c < componentCount; ++c)
		{
			const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
			const FE_element_field_template *eft = mft->getElementfieldtemplate(element->getIndex());
			if (!eft)
				break; // field not defined on this element
			if (eft != lastEft)
			{
				if (eft->getNumberOfLocalNodes() > 0)
				{
					const FE_mesh_element_field_template_data *meshEftData = this->mesh->getElementfieldtemplateData(eft);
					const DsLabelIndex *nodeIndexes = meshEftData->getElementNodeIndexes(element->getIndex());
					if (!nodeIndexes)
					{
						display_message(WARNING_MESSAGE, "EXWriter::writeElementHeader.  Missing node indexes");
					}
					this->headerElementNodePacking->packEftNodes(eft, nodeIndexes);
				}
				if (eft->getNumberOfLocalScaleFactors() > 0)
				{
					lastEft = eft;
					auto eftIter = this->headerScalingEfts.begin();
					for (; eftIter != this->headerScalingEfts.end(); ++eftIter)
					{
						if (*eftIter == eft)
							break;
					}
					if (eftIter == this->headerScalingEfts.end())
						this->headerScalingEfts.push_back(eft);
				}
			}
		}
		if (c == componentCount)
			this->headerFields.push_back(field);
	}

	(*this->output_file) << "#Scale factor sets=" << this->headerScalingEfts.size() << "\n";
	int scaleFactorSetIndex = 0;
	for (auto eftIter = this->headerScalingEfts.begin(); eftIter != this->headerScalingEfts.end(); ++eftIter)
	{
		++scaleFactorSetIndex;
		(*this->output_file) << "  scaling" << scaleFactorSetIndex << ", #Scale factors=" << (*eftIter)->getNumberOfLocalScaleFactors()
			<< ", identifiers=\"";
		if (!this->writeEftScaleFactorIdentifiers(*eftIter))
			return false;
		(*this->output_file) << "\"\n";
	}
	(*this->output_file) << "#Nodes=" << this->headerElementNodePacking->getTotalNodeCount() << "\n";
	(*this->output_file) << "#Fields=" << this->headerFields.size() << "\n";
	int fieldIndex = 0;
	for (auto fieldIter = this->headerFields.begin(); fieldIter != this->headerFields.end(); ++fieldIter)
	{
		++fieldIndex;
		FE_field *field = *fieldIter;
		if (!this->writeElementHeaderField(element, fieldIndex, field))
			return false;
	}

	if (!this->writeOptionalFieldValues())
	{
		return false;
	}
	return true;
}

bool EXWriter::writeElementFieldComponentValues(cmzn_element *element,
	FE_field *field, int componentNumber)
{
	const FE_mesh_field_data *meshFieldData = field->getMeshFieldData(this->mesh);
	const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(componentNumber);
	const FE_element_field_template *eft = mft->getElementfieldtemplate(element->getIndex());
	const int valueCount = eft->getNumberOfElementDOFs();
	FE_mesh_field_data::ComponentBase *componentBase = meshFieldData->getComponentBase(componentNumber);
	Value_type valueType = get_FE_field_value_type(field);

	const int *gridNumberInXi = eft->getLegacyGridNumberInXi();
	// have new line every number-of-grid-points-in-xi1
	const int columnCount = (gridNumberInXi) ? gridNumberInXi[0] + 1 : valueCount;

	switch (valueType)
	{
	case FE_VALUE_VALUE:
	{
		auto component = static_cast<FE_mesh_field_data::Component<FE_value>*>(componentBase);
		const FE_value *values = component->getElementValues(element->getIndex(), valueCount);
		if (!values)
		{
			display_message(ERROR_MESSAGE, "EXWriter::writeElementFieldComponentValues.  Missing real values");
			return false;
		}
		char tmpString[100];
		for (int v = 0; v < valueCount; ++v)
		{
			sprintf(tmpString, " %" FE_VALUE_STRING, values[v]);
			(*this->output_file) << tmpString;
			if (0 == ((v + 1) % columnCount))
				(*this->output_file) << "\n";
		}
		// extra newline if not multiple of number_of_columns
		if (0 != (valueCount % columnCount))
			(*this->output_file) << "\n";
		break;
	}
	case INT_VALUE:
	{
		auto component = static_cast<FE_mesh_field_data::Component<int>*>(componentBase);
		const int *values = component->getElementValues(element->getIndex(), valueCount);
		if (!values)
		{
			display_message(ERROR_MESSAGE, "EXWriter::writeElementFieldComponentValues.  Missing int values");
			return false;
		}
		for (int v = 0; v < valueCount; ++v)
		{
			(*this->output_file) << " " << values[v];
			if (0 == ((v + 1) % columnCount))
				(*this->output_file) << "\n";
		}
		// extra newline if not multiple of number_of_columns
		if (0 != (valueCount % columnCount))
			(*this->output_file) << "\n";
		break;
	}
	default:
	{
		display_message(ERROR_MESSAGE,
			"EXWriter::writeElementFieldComponentValues.  Unsupported value type %s", Value_type_string(valueType));
		return false;
	} break;
	}
	return true;
}

/**
 * Writes out an element to stream in format:

Element: 1
  Faces:
  1 2 3 4 5 6
  Values:
  1 2 4 16 32 64 128 6 7 8 7 8 9 8 9 10 9 10 11 10 11 12 7 8 9 8 9 10 9 10 11 10
  11 12 11 12 13 8 9 10 9 10 11 10 11 12 11 12 13 12 13 14 9 10 11 10 11 12 11 12
  13 12 13 14 13 14 15
  Nodes:
  1 2 3 4 5 6 7 8
  Scale factors:
  0.100000E+01  0.100000E+01  0.100000E+01  0.100000E+01  0.100000E+01
  0.100000E+01  0.100000E+01  0.100000E+01

 * Notes:
 * Faces are only output for elements with the faces array allocated. Missing
 * faces are given the identifier -1 as expected by read_FE_element.
 * Values, Nodes and Scale Factors are only output if present for element fields.
 */
bool EXWriter::writeElement(cmzn_element *element)
{
	(*this->output_file) << "Element: " << element->getIdentifier() << "\n";

	if (this->writeGroupOnly)
		return true;

	// Faces: if defined
	const FE_mesh::ElementShapeFaces *elementShapeFaces = this->mesh->getElementShapeFacesConst(element->getIndex());
	if (!elementShapeFaces)
	{
		display_message(ERROR_MESSAGE, "EXWriter::writeElement.  Missing ElementShapeFaces");
		return false;
	}
	FE_mesh *faceMesh = this->mesh->getFaceMesh();
	if (faceMesh)
	{
		const int faceCount = elementShapeFaces->getFaceCount();
		const DsLabelIndex *faceIndexes;
		if ((0 < faceCount) && (faceIndexes = elementShapeFaces->getElementFaces(element->getIndex())))
		{
			(*this->output_file) << " Faces:\n";
			for (int i = 0; i < faceCount; ++i)
			{
				if (faceIndexes[i] >= 0)
					(*this->output_file) << " " << faceMesh->getElementIdentifier(faceIndexes[i]);
				else
					(*this->output_file) << " -1"; // face not set; can't use 0 as it is a valid identifier
			}
			(*this->output_file) << "\n";
		}
	}

	// Values: if writing any element-based fields
	bool firstElementBasedField = true;
	for (auto fieldIter = this->headerFields.begin(); fieldIter != this->headerFields.end(); ++fieldIter)
	{
		FE_field *field = *fieldIter;
		if (GENERAL_FE_FIELD != get_FE_field_FE_field_type(field))
			continue;
		const FE_mesh_field_data *meshFieldData = field->getMeshFieldData(this->mesh);
		const int componentCount = get_FE_field_number_of_components(field);
		for (int c = 0; c < componentCount; ++c)
		{
			const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(c);
			const FE_element_field_template *eft = mft->getElementfieldtemplate(element->getIndex());
			if (eft->getNumberOfElementDOFs() > 0)
			{
				if (firstElementBasedField)
				{
					(*this->output_file) << " Values :\n";
					firstElementBasedField = false;
				}
				if (!this->writeElementFieldComponentValues(element, field, c))
					return false;
			}
		}
	}

	// Nodes: if any
	if (this->headerElementNodePacking->getTotalNodeCount() > 0)
	{
		FE_nodeset *nodeset = this->mesh->getNodeset();
		(*this->output_file) << " Nodes:\n";
		int index = 0;
		const FE_element_field_template *eft;
		while (0 != (eft = this->headerElementNodePacking->getFirstEftAtIndex(index)))
		{
			const FE_mesh_element_field_template_data *meshEftData = this->mesh->getElementfieldtemplateData(eft);
			const int nodeCount = eft->getNumberOfLocalNodes();
			const DsLabelIndex *nodeIndexes = meshEftData->getElementNodeIndexes(element->getIndex());
			if (nodeIndexes)
			{
				for (int n = 0; n < nodeCount; ++n)
				{
					(*this->output_file) << " " << nodeset->getNodeIdentifier(nodeIndexes[n]);
				}
			}
			else
			{
				for (int n = 0; n < nodeCount; ++n)
				{
					(*this->output_file) << " -1";
				}
			}
			++index;
		}
		(*this->output_file) << "\n";
	}

	// Scale factors: if any scale factor sets being output
	if (this->headerScalingEfts.size() > 0)
	{
		int scaleFactorNumber = 0;
		char tmpString[100];
		(*this->output_file) << " Scale factors:\n";
		for (auto eftIter = this->headerScalingEfts.begin(); eftIter != this->headerScalingEfts.end(); ++eftIter)
		{
			const FE_element_field_template *eft = *eftIter;
			FE_mesh_element_field_template_data *meshEftData = this->mesh->getElementfieldtemplateData(eft);
			const int scaleFactorCount = eft->getNumberOfLocalScaleFactors();
			int result = CMZN_OK;
			const DsLabelIndex *scaleFactorIndexes = meshEftData->getOrCreateElementScaleFactorIndexes(result, element->getIndex());
			if (!scaleFactorIndexes)
			{
				display_message(WARNING_MESSAGE, "EXWriter::writeElement.  Missing scale factors for element %d", element->getIdentifier());
			}
			for (int s = 0; s < scaleFactorCount; ++s)
			{
				++scaleFactorNumber;
				sprintf(tmpString, "%" FE_VALUE_STRING, (scaleFactorIndexes) ? mesh->getScaleFactor(scaleFactorIndexes[s]) : 0.0);
				(*this->output_file) << " " << tmpString;
				if ((0 < FE_VALUE_MAX_OUTPUT_COLUMNS)
					&& (0 == (scaleFactorNumber % FE_VALUE_MAX_OUTPUT_COLUMNS)))
				{
					(*this->output_file) << "\n";
				}
			}
			// extra new line if not multiple of FE_VALUE_MAX_OUTPUT_COLUMNS values
			if ((FE_VALUE_MAX_OUTPUT_COLUMNS <= 0)
				|| (0 != (scaleFactorNumber % FE_VALUE_MAX_OUTPUT_COLUMNS)))
			{
				(*this->output_file) << "\n";
			}
		}
	}
	return true;
}

/**
 * @return  True if specification of what to output, optionally including the
 * field order info, indicates the <element> is to be written.
 */
bool EXWriter::elementIsToBeWritten(cmzn_element *element)
{
	if (!((element) && ((this->write_criterion == FE_WRITE_COMPLETE_GROUP) || (this->field_order_info))))
	{
		display_message(ERROR_MESSAGE,
			"EXWriter::elementIsToBeWritten.  Invalid argument(s)");
		return false;
	}
	switch (this->write_criterion)
	{
		case FE_WRITE_COMPLETE_GROUP:
		{
		} break;
		case FE_WRITE_WITH_ALL_LISTED_FIELDS:
		{
			const int number_of_fields = get_FE_field_order_info_number_of_fields(this->field_order_info);
			for (int i = 0; i < number_of_fields; ++i)
			{
				struct FE_field *field = get_FE_field_order_info_field(this->field_order_info, i);
				if (!FE_field_is_defined_in_element(field, element))
					return false;
			}
		} break;
		case FE_WRITE_WITH_ANY_LISTED_FIELDS:
		{
			const int number_of_fields = get_FE_field_order_info_number_of_fields(this->field_order_info);
			for (int i = 0; i < number_of_fields; ++i)
			{
				struct FE_field *field = get_FE_field_order_info_field(this->field_order_info, i);
				if (FE_field_is_defined_in_element(field, element))
					return true;
			}
			return false;
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"EXWriter::elementIsToBeWritten.  Unknown write_criterion");
			return false;
		} break;
	}
	return true;
}

/** @return true if any fields are to be written for the element. */
bool EXWriter::elementHasFieldsToWrite(cmzn_element *element)
{
	if (!this->field_order_info)
		return (0 < get_FE_element_number_of_fields(element));
	const int number_of_fields = get_FE_field_order_info_number_of_fields(field_order_info);
	for (int i = 0; i < number_of_fields; ++i)
	{
		struct FE_field *field = get_FE_field_order_info_field(field_order_info, i);
		if (FE_field_is_defined_in_element_not_inherited(field, element))
			return true;
	}
	return false;
}

/**
 * Returns true if element has matching field definition to last element whose
 * field header was output, or if there was no last element.
 * If field_order_info is supplied only those fields listed in it are compared.
 * Note that even if this returns true, need to check element nodes are
 * packed identically for fields written together.
 */
bool EXWriter::elementFieldsMatchLastElement(cmzn_element *element)
{
	if (!this->headerElement)
		return false;
	if (!this->field_order_info)
		return (0 != equivalent_FE_fields_in_elements(element, this->headerElement));
	const int number_of_fields = get_FE_field_order_info_number_of_fields(field_order_info);
	for (int f = 0; f < number_of_fields; ++f)
	{
		struct FE_field *field = get_FE_field_order_info_field(field_order_info, f);
		if (!equivalent_FE_field_in_elements(field, element, this->headerElement))
			return false;
	}
	return true;
}

/**
 * Writes element to the stream. If the element template is different from the
 * header element then a new header is written out. If the element
 * has no fields, only the shape is output in the header.
 */
bool EXWriter::writeElementExt(cmzn_element *element)
{
	if (!element)
	{
		display_message(ERROR_MESSAGE, "EXWriter::writeElementExt.  Invalid argument(s)");
		return false;
	}
	if (!this->elementIsToBeWritten(element))
		return true;
	// work out if shape or field header have changed from last element
	FE_element_shape *elementShape = element->getElementShape();
	if (!elementShape)
		return false;
	bool newShape = true;
	bool newFieldHeader = true;
	if (this->headerElement)
	{
		newShape = (elementShape != this->lastElementShape);
		if (newShape)
		{
			newFieldHeader = this->elementHasFieldsToWrite(element); // reader must assume a blank, no-field template
		}
		else
		{
			newFieldHeader = !this->EXWriter::elementFieldsMatchLastElement(element);
			if (!newFieldHeader)
			{
				ElementNodePacking *elementNodePacking = this->createElementNodePacking(element);
				newFieldHeader = !elementNodePacking->matches(*this->headerElementNodePacking);
				delete elementNodePacking;
			}
		}
	}
	if (newShape)
	{
		if (!this->writeElementShape(elementShape))
			return false;
	}
	if (newFieldHeader)
	{
		if (!this->writeElementHeader(element))
			return false;
	}
	if (!this->writeElement(element))
		return false;
	return true;
}

/**
 * Writes a node field to stream. After the field header is written with
 * write_FE_field_header, value index, derivative and version info are output on
 * following lines, one per component:
 * COMPONENT_NAME.  #Values=~ (valueType(versions) ...)
 */
bool EXWriter::writeNodeHeaderField(cmzn_node *node, int fieldIndex, FE_field *field)
{
	if (!this->writeFieldHeader(fieldIndex, field))
	{
		return false;
	}
	FE_field_type fe_field_type = get_FE_field_FE_field_type(field);
	const int componentCount = get_FE_field_number_of_components(field);
	const FE_node_field *node_field = node->getNodeField(field);
	if (!node_field)
	{
		display_message(ERROR_MESSAGE, "EXWriter::writeNodeHeaderField.  Field is not defined at node");
		return false;
	}
	for (int c = 0; c < componentCount; ++c)
	{
		char *componentName = get_FE_field_component_name(field, c);
		if (componentName)
		{
			(*this->output_file) << " " << componentName << ".";
			DEALLOCATE(componentName);
		}
		else
		{
			(*this->output_file) << " " << c + 1 << ".";
		}
		if (fe_field_type != GENERAL_FE_FIELD)
		{
			// constant and indexed fields: no further component information
			(*this->output_file) << "\n";
			continue;
		}
		const FE_node_field_template& nft = *(node_field->getComponent(c));
		const int valuesCount = nft.getTotalValuesCount();
		(*this->output_file) << " #Values=" << valuesCount << " (";
		const int valueLabelsCount = nft.getValueLabelsCount();
		for (int d = 0; d < valueLabelsCount; ++d)
		{
			if (d > 0)
			{
				(*this->output_file) << ",";
			}
			const cmzn_node_value_label valueLabel = nft.getValueLabelAtIndex(d);
			const int versionsCount = nft.getVersionsCountAtIndex(d);
			(*this->output_file) << ENUMERATOR_STRING(cmzn_node_value_label)(valueLabel);
			if (versionsCount > 1)
			{
				(*this->output_file) << "(" << versionsCount << ")";
			}
		}
		(*this->output_file) << ")\n";
	}
	return true;
}

/**
 * Writes out the nodal values. Each component starts on a new line.
 * In the new EX2 format, all versions for a given derivative are
 * consecutively output.
 * Only call for general field defined on node - this is not checked.
 */
bool EXWriter::writeNodeFieldValues(cmzn_node *node, FE_field *field)
{
	const int componentCount = get_FE_field_number_of_components(field);
	const FE_node_field *node_field = node->getNodeField(field);
	if (!node_field)
	{
		display_message(ERROR_MESSAGE, "EXWriter::writeNodeFieldValues.  Field %s not defined at node %d",
			get_FE_field_name(field), get_FE_node_identifier(node));
		return false;
	}
	const int maximumValuesCount = node_field->getMaximumComponentTotalValuesCount();
	const enum Value_type valueType = get_FE_field_value_type(field);
	switch (valueType)
	{
	case ELEMENT_XI_VALUE:
	{
		FE_value xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
		cmzn_element *element;
		const FE_mesh *hostMesh = field->getElementXiHostMesh();
		if (!hostMesh)
		{
			display_message(ERROR_MESSAGE, "EXWriter::writeNodeFieldValues.  Missing host mesh for element xi field");
			return false;
		}
		// should only be one component, no derivatives and versions
		for (int c = 0; c < componentCount; ++c)
		{
			if (!get_FE_nodal_element_xi_value(node, field, /*component_number*/c, &element, xi))
			{
				display_message(ERROR_MESSAGE, "EXWriter::writeNodeFieldValues.  Could not get element_xi value");
				return false;
			}
			if (!this->writeElementXiValue(hostMesh, element ? element->getIndex() : DS_LABEL_IDENTIFIER_INVALID, xi))
			{
				return false;
			}
			(*this->output_file) << "\n";
		}
	} break;
	case FE_VALUE_VALUE:
	{
		char tmpString[100];
		std::vector<FE_value> valuesVector(maximumValuesCount);
		FE_value *values = valuesVector.data();
		for (int c = 0; c < componentCount; ++c)
		{
			const FE_node_field_template *nft = node_field->getComponent(c);
			const int valuesCount = nft->getTotalValuesCount();
			// EX2 format matches internal storage with versions consecutive for each value label
			if (CMZN_OK != cmzn_node_get_field_component_FE_value_values(node, field, c, this->time, maximumValuesCount, values))
			{
				display_message(ERROR_MESSAGE, "EXWriter::writeNodeFieldValues.  "
					"Failed to get FE_value values for field %s component %d at node %d",
					get_FE_field_name(field), c + 1, get_FE_node_identifier(node));
				return false;
			}
			for (int v = 0; v < valuesCount; ++v)
			{
				sprintf(tmpString, "%" FE_VALUE_STRING, values[v]);
				(*this->output_file) << " " << tmpString;
			}
			if (valuesCount)
			{
				(*this->output_file) << "\n";
			}
		}
	} break;
	case INT_VALUE:
	{
		std::vector<int> valuesVector(maximumValuesCount);
		int *values = valuesVector.data();
		for (int c = 0; c < componentCount; ++c)
		{
			const FE_node_field_template *nft = node_field->getComponent(c);
			const int valuesCount = nft->getTotalValuesCount();
			// EX2 format matches internal storage with versions consecutive for each value label
			if (CMZN_OK != cmzn_node_get_field_component_int_values(node, field, c, this->time, maximumValuesCount, values))
			{
				display_message(ERROR_MESSAGE, "EXWriter::writeNodeFieldValues.  "
					"Failed to get FE_value values for field %s component %d at node %d",
					get_FE_field_name(field), c + 1, get_FE_node_identifier(node));
				return false;
			}
			for (int v = 0; v < valuesCount; ++v)
			{
				(*this->output_file) << " " << values[v];
			}
			if (valuesCount)
			{
				(*this->output_file) << "\n";
			}
		}
	} break;
	case STRING_VALUE:
	{
		char *the_string;

		// should only be one component, no derivatives and versions
		for (int c = 0; c < componentCount; ++c)
		{
			if (get_FE_nodal_string_value(node, field, /*component_number*/c, &the_string))
			{
				if (the_string)
				{
					make_valid_token(&the_string);
					(*this->output_file) << " " << the_string;
					DEALLOCATE(the_string);
				}
				else
				{
					/* empty string */
					(*this->output_file) << " \"\"";
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"EXWriter::writeNodeFieldValues.  Could not get string");
			}
			(*this->output_file) << "\n";
		}
	} break;
	default:
	{
		display_message(ERROR_MESSAGE,
			"EXWriter::writeNodeFieldValues.  Value type %s not supported",
			Value_type_string(valueType));
	} break;
	}
	return true;
}

/** Writes out a node to stream */
bool EXWriter::writeNode(cmzn_node *node)
{
	(*this->output_file) << "Node: " << get_FE_node_identifier(node) << "\n";

	if (this->writeGroupOnly)
		return true;

	// values, if writing any general fields
	for (auto fieldIter = this->headerFields.begin(); fieldIter != this->headerFields.end(); ++fieldIter)
	{
		FE_field *field = *fieldIter;
		if ((GENERAL_FE_FIELD == get_FE_field_FE_field_type(field))
			&& !this->writeNodeFieldValues(node, field))
			return false;
	}
	return true;
}


/**
 * @return  True if specification of what to output, optionally including the
 * field order info, indicates the node is to be written.
 */
bool EXWriter::nodeIsToBeWritten(cmzn_node *node)
{
	if (!((node) && ((this->write_criterion == FE_WRITE_COMPLETE_GROUP) || (this->field_order_info))))
	{
		display_message(ERROR_MESSAGE,
			"EXWriter::nodeIsToBeWritten.  Invalid argument(s)");
		return false;
	}
	switch (this->write_criterion)
	{
	case FE_WRITE_COMPLETE_GROUP:
	{
	} break;
	case FE_WRITE_WITH_ALL_LISTED_FIELDS:
	{
		const int number_of_fields = get_FE_field_order_info_number_of_fields(this->field_order_info);
		for (int i = 0; i < number_of_fields; ++i)
		{
			struct FE_field *field = get_FE_field_order_info_field(this->field_order_info, i);
			if (!(node->getNodeField(field)))
				return false;
		}
	} break;
	case FE_WRITE_WITH_ANY_LISTED_FIELDS:
	{
		const int number_of_fields = get_FE_field_order_info_number_of_fields(this->field_order_info);
		for (int i = 0; i < number_of_fields; ++i)
		{
			struct FE_field *field = get_FE_field_order_info_field(this->field_order_info, i);
			if (node->getNodeField(field))
				return true;
		}
		return false;
	} break;
	default:
	{
		display_message(ERROR_MESSAGE,
			"EXWriter::nodeIsToBeWritten.  Unknown write_criterion");
		return false;
	} break;
	}
	return true;
}

static int FE_nodes_have_same_header(struct FE_node *node_1,
	struct FE_node *node_2, struct FE_field_order_info *field_order_info)
/*******************************************************************************
LAST MODIFIED : 10 September 2001

DESCRIPTION :
Returns true if <node_1> and <node_2> can be written to file with the
same fields header. If <field_order_info> is supplied only those fields
listed in it are compared.
==============================================================================*/
{
	int i, number_of_fields, return_code;
	struct FE_field *field;

	ENTER(FE_nodes_have_same_header);
	if (node_1 && node_2)
	{
		if (field_order_info)
		{
			return_code = 1;
			number_of_fields =
				get_FE_field_order_info_number_of_fields(field_order_info);
			for (i = 0; (i < number_of_fields) && return_code; i++)
			{
				if (!((field = get_FE_field_order_info_field(field_order_info, i)) &&
					equivalent_FE_field_at_nodes(field, node_1, node_2)))
				{
					return_code = 0;
				}
			}
		}
		else
		{
			return_code = equivalent_FE_fields_at_nodes(node_1, node_2);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"FE_nodes_have_same_header.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* FE_nodes_have_same_header */





  /**
  * Writes the node field information header for node. If the
  * field_order_info is supplied the header is restricted to include only
  * components/fields/bases for it.
  */
bool EXWriter::writeNodeHeader(cmzn_node *node)
{
	if (!node)
	{
		display_message(ERROR_MESSAGE, "EXWriter::writeNodeHeader.  Invalid argument(s)");
		return false;
	}

	this->headerNode = node;
	this->headerFields.clear();

	// make list of fields in header
	for (auto fieldIter = this->writableFields.begin(); fieldIter != this->writableFields.end(); ++fieldIter)
	{
		FE_field *field = *fieldIter;
		if (node->getNodeField(field))
			this->headerFields.push_back(field);
	}

	(*this->output_file) << "#Fields=" << this->headerFields.size() << "\n";
	int fieldIndex = 0;
	for (auto fieldIter = this->headerFields.begin(); fieldIter != this->headerFields.end(); ++fieldIter)
	{
		++fieldIndex;
		FE_field *field = *fieldIter;
		if (!this->writeNodeHeaderField(node, fieldIndex, field))
			return false;
	}
	if (!this->writeOptionalFieldValues())
	{
		return false;
	}
	return true;
}

/**
 * Writes a node to the given file.  If the fields defined at the node are
 * different from the last node (taking into account whether a selection of fields
 * has been selected for output) then the header is written out.
 */
bool EXWriter::writeNodeExt(cmzn_node *node)
{
	if (!node)
	{
		display_message(ERROR_MESSAGE, "EXWriter::writeNodeExt.  Invalid argument(s)");
		return false;
	}
	if (!this->nodeIsToBeWritten(node))
		return true;

	if ((0 == this->headerNode)
		|| !FE_nodes_have_same_header(node, this->headerNode, field_order_info))
	{
		if (!this->writeNodeHeader(node))
			return false;
	}
	if (!this->writeNode(node))
		return false;
	return true;
}

static int write_cmzn_region_content(ostream *output_file,
	struct cmzn_region *region, cmzn_field_group_id group,
	int write_elements, int write_nodes, int write_data,
	enum FE_write_fields_mode write_fields_mode,
	int number_of_field_names, char **field_names, int *field_names_counter,
	FE_value time, enum FE_write_criterion write_criterion,
	bool writeGroupOnly = false)
/*******************************************************************************
LAST MODIFIED : 27 February 2003

DESCRIPTION :
Writes <base_fe_region> to the <output_file>.
If <field_order_info> is NULL, all element fields are written in the default,
alphabetical order.
If <field_order_info> is empty, only element identifiers are output.
If <field_order_info> contains fields, they are written in that order.
Additionally, the <write_criterion> controls output as follows:
FE_WRITE_COMPLETE_GROUP = write all elements in the group (the default);
FE_WRITE_WITH_ALL_LISTED_FIELDS =
  write only elements with all listed fields defined;
FE_WRITE_WITH_ANY_LISTED_FIELDS =
  write only elements with any listed fields defined.
==============================================================================*/
{
	int return_code;

	ENTER(write_cmzn_region_content);
	if (output_file && region)
	{
		cmzn_fieldmodule_id field_module = cmzn_region_get_fieldmodule(region);
		FE_region *fe_region = region->get_FE_region();
		return_code = 1;
		FE_field_order_info *field_order_info = CREATE(FE_field_order_info)();

		if (write_fields_mode != FE_WRITE_NO_FIELDS)
		{
			if (write_fields_mode == FE_WRITE_ALL_FIELDS)
			{
				// get list of all fields in default alphabetical order
				return_code = FE_region_for_each_FE_field(fe_region,
					FE_field_add_to_FE_field_order_info, (void *)field_order_info);
				FE_field_order_info_prioritise_indexer_fields(field_order_info);
			}
			else if ((0 < number_of_field_names) && field_names && field_names_counter)
			{
				for (int i = 0; (i < number_of_field_names) && return_code; i++)
				{
					if (field_names[i])
					{
						struct FE_field *fe_field = FE_region_get_FE_field_from_name(fe_region, field_names[i]);
						if (fe_field)
						{
							++(field_names_counter[i]);
							return_code = add_FE_field_order_info_field(field_order_info, fe_field);
						}
					}
					else
					{
						display_message(ERROR_MESSAGE, "write_cmzn_region_content.  NULL field name");
						return_code = 0;
					}
				}
			}
			else if (0 < number_of_field_names)
			{
				display_message(ERROR_MESSAGE, "write_cmzn_region_content.  Missing field names array");
				return_code = 0;
			}
		}

		if (return_code)
		{
			EXWriter exWriter(output_file, write_criterion, field_order_info, time);
			if (writeGroupOnly)
				exWriter.setWriteGroupOnly();
			// write nodes then elements then data last since future plan is to remove the feature
			// where the same field can be defined simultaneously on nodes & elements and also data.
			// To migrate the first one will use the actual field name and the other will need to
			// qualify it, and we want the qualified one to be the datapoints.
			if (write_nodes)
			{
				FE_nodeset *feNodeset = FE_region_find_FE_nodeset_by_field_domain_type(fe_region, CMZN_FIELD_DOMAIN_TYPE_NODES);
				exWriter.setNodeset(feNodeset);
				cmzn_nodeset_id nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(field_module,
					CMZN_FIELD_DOMAIN_TYPE_NODES);
				if (group)
				{
					cmzn_field_node_group_id node_group = cmzn_field_group_get_field_node_group(group, nodeset);
					cmzn_nodeset_destroy(&nodeset);
					nodeset = cmzn_nodeset_group_base_cast(cmzn_field_node_group_get_nodeset_group(node_group));
					cmzn_field_node_group_destroy(&node_group);
				}
				if (nodeset && (cmzn_nodeset_get_size(nodeset) > 0))
				{
					(*output_file) << "!#nodeset ";
					exWriter.writeSafeName(feNodeset->getName());
					(*output_file) << "\nShape. Dimension=0\n";
					cmzn_nodeiterator_id iter = cmzn_nodeset_create_nodeiterator(nodeset);
					cmzn_node_id node = 0;
					while (0 != (node = cmzn_nodeiterator_next_non_access(iter)))
					{
						if (!exWriter.writeNodeExt(node))
						{
							return_code = 0;
							break;
						}
					}
					cmzn_nodeiterator_destroy(&iter);
					cmzn_nodeset_destroy(&nodeset);
				}
			}
			if (write_elements)
			{
				const int highest_dimension = FE_region_get_highest_dimension(fe_region);
				/* write 1-D, 2-D then 3-D so lines and faces precede elements */
				for (int dimension = 1; dimension <= highest_dimension; dimension++)
				{
					if ((dimension == 1 && (write_elements & CMZN_FIELD_DOMAIN_TYPE_MESH1D)) ||
						(dimension == 2 && (write_elements & CMZN_FIELD_DOMAIN_TYPE_MESH2D)) ||
						(dimension == 3 && (write_elements & CMZN_FIELD_DOMAIN_TYPE_MESH3D)) ||
						(dimension == highest_dimension &&
						(write_elements & CMZN_FIELD_DOMAIN_TYPE_MESH_HIGHEST_DIMENSION)))
					{
						cmzn_mesh_id mesh = cmzn_fieldmodule_find_mesh_by_dimension(field_module, dimension);
						if (group)
						{
							cmzn_field_element_group_id element_group = cmzn_field_group_get_field_element_group(group, mesh);
							cmzn_mesh_destroy(&mesh);
							mesh = cmzn_mesh_group_base_cast(cmzn_field_element_group_get_mesh_group(element_group));
							cmzn_field_element_group_destroy(&element_group);
						}
						if (mesh && (cmzn_mesh_get_size(mesh) > 0))
						{
							FE_mesh *feMesh = FE_region_find_FE_mesh_by_dimension(fe_region, dimension);
							(*output_file) << "!#mesh ";
							exWriter.writeSafeName(feMesh->getName());
							(*output_file) << ", dimension=" << feMesh->getDimension();
							if (feMesh->getFaceMesh())
							{
								(*output_file) << ", face mesh=";
								exWriter.writeSafeName(feMesh->getFaceMesh()->getName());
							}
							if (feMesh->getNodeset())
							{
								(*output_file) << ", nodeset=";
								exWriter.writeSafeName(feMesh->getNodeset()->getName());
							}
							(*output_file) << "\n";
							exWriter.setMesh(feMesh);
							cmzn_elementiterator_id iter = cmzn_mesh_create_elementiterator(mesh);
							cmzn_element_id element = 0;
							while (0 != (element = cmzn_elementiterator_next_non_access(iter)))
							{
								if (!exWriter.writeElementExt(element))
								{
									return_code = 0;
									break;
								}
							}
							cmzn_elementiterator_destroy(&iter);
							cmzn_mesh_destroy(&mesh);
						}
					}
				}
			}
			if (write_data)
			{
				FE_nodeset *feNodeset = FE_region_find_FE_nodeset_by_field_domain_type(fe_region, CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS);
				exWriter.setNodeset(feNodeset);
				cmzn_nodeset_id nodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(field_module,
					CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS);
				if (group)
				{
					cmzn_field_node_group_id node_group = cmzn_field_group_get_field_node_group(group, nodeset);
					cmzn_nodeset_destroy(&nodeset);
					nodeset = cmzn_nodeset_group_base_cast(cmzn_field_node_group_get_nodeset_group(node_group));
					cmzn_field_node_group_destroy(&node_group);
				}
				if (nodeset && (cmzn_nodeset_get_size(nodeset) > 0))
				{
					(*output_file) << "!#nodeset ";
					exWriter.writeSafeName(feNodeset->getName());
					(*output_file) << "\nShape. Dimension=0\n";
					cmzn_nodeiterator_id iter = cmzn_nodeset_create_nodeiterator(nodeset);
					cmzn_node_id node = 0;
					while (0 != (node = cmzn_nodeiterator_next_non_access(iter)))
					{
						if (!exWriter.writeNodeExt(node))
						{
							return_code = 0;
							break;
						}
					}
					cmzn_nodeiterator_destroy(&iter);
					cmzn_nodeset_destroy(&nodeset);
				}
			}
		}
		if (!return_code)
		{
			display_message(ERROR_MESSAGE, "write_cmzn_region_content.  Failed");
			return_code = 0;
		}
		if (field_order_info)
		{
			DESTROY(FE_field_order_info)(&field_order_info);
		}
		cmzn_fieldmodule_destroy(&field_module);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"write_cmzn_region_content.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

/***************************************************************************//**
 * Recursively writes the finite element fields in region tree to file.
 * Notes:
 * - the master region for a group must be the parent region.
 * - element_xi values currently restricted to being in the root_region.
 * 
 * @param output_file  The file to write region and field data to.
 * @param region  The region to write.
 * @param parent_region_output  Pointer to parent region if just output to same
 *   file. Caller should set to NULL on first call.
 * @param root_region  The root region of any data to be written. Need not be
 *   the true root of region hierarchy, but region paths in file are relative to
 *   this region.
 * @param write_elements  If set, write elements and element fields to file.
 * @param write_nodes  If set, write nodes and node fields to file.
 * @param write_data  If set, write data and data fields to file. May only use
 *   if write_elements and write_nodes are 0.
 * @param write_fields_mode  Controls which fields are written to file.
 *   If mode is FE_WRITE_LISTED_FIELDS then:
 *   - Number/list of field_names must be supplied;
 *   - Field names not used in a region are ignored;
 *   - Warnings are given for any field names not used in any output region.
 * @param number_of_field_names  The number of names in the field_names array.
 * @param field_names  Array of field names.
 * @param field_names_counter  Array of integers of same length as field_names
 *   incremented whenever the respective field name is matched.
 * @param write_criterion  Controls which objects are written. Some modes
 *   limit output to nodes or objects with any or all listed fields defined.
 * @param write_recursion  Controls whether sub-regions and sub-groups are
 *   recursively written.
 */
static int write_cmzn_region(ostream *output_file,
	struct cmzn_region *region, const char * group_name,
	struct cmzn_region *root_region,
	int write_elements, int write_nodes, int write_data,
	enum FE_write_fields_mode write_fields_mode,
	int number_of_field_names, char **field_names, int *field_names_counter,
	FE_value time,
	enum FE_write_criterion write_criterion,
	enum cmzn_streaminformation_region_recursion_mode recursion_mode)
{
	int return_code;

	ENTER(write_cmzn_region);
	if (output_file && region && root_region)
	{
		return_code = 1;

		// write region path and/or group name */
		cmzn_field_group_id group = 0;
		if (group_name)
		{
			cmzn_fieldmodule_id fieldmodule =  cmzn_region_get_fieldmodule(region);
			cmzn_field_id field = cmzn_fieldmodule_find_field_by_name(fieldmodule, group_name);
			if (field)
			{
				group = cmzn_field_cast_group(field);
				cmzn_field_destroy(&field);
			}
			cmzn_fieldmodule_destroy(&fieldmodule);
		}

		if (!group_name || group)
		{
			if (!group || (region != root_region))
			{
				char *region_path = region->getRelativePath(root_region);
				size_t len = strlen(region_path);
				if ((1 < len) && (region_path[len - 1] == CMZN_REGION_PATH_SEPARATOR_CHAR))
				{
					region_path[len - 1] = '\0';
				}
				(*output_file) << "Region: " << region_path << "\n";
				DEALLOCATE(region_path);
			}
			if (group)
			{
				char *group_name = cmzn_field_get_name(cmzn_field_group_base_cast(group));
				(*output_file) << " Group name: " << group_name << "\n";
				DEALLOCATE(group_name);
			}

			// write finite element fields for this region
			if (return_code)
			{
				return_code = write_cmzn_region_content(output_file, region, group,
					write_elements, write_nodes, write_data,
					write_fields_mode, number_of_field_names, field_names,
					field_names_counter, time, write_criterion);
			}
		}

		if (return_code && !group_name && recursion_mode == CMZN_STREAMINFORMATION_REGION_RECURSION_MODE_ON)
		{
			// write group members
			cmzn_fieldmodule_id field_module = cmzn_region_get_fieldmodule(region);
			cmzn_fielditerator_id field_iter = cmzn_fieldmodule_create_fielditerator(field_module);
			cmzn_field_id field = 0;
			while ((0 != (field = cmzn_fielditerator_next_non_access(field_iter))) && return_code)
			{
				cmzn_field_group_id output_group = cmzn_field_cast_group(field);
				if (output_group)
				{
					char *group_name = cmzn_field_get_name(field);
					(*output_file) << " Group name: " << group_name << "\n";
					DEALLOCATE(group_name);
					return_code = write_cmzn_region_content(output_file, region, output_group,
						write_elements, write_nodes, write_data,
						FE_WRITE_NO_FIELDS, number_of_field_names, field_names,
						field_names_counter, time, write_criterion, /*writeGroupOnly*/true);
					cmzn_field_group_destroy(&output_group);
				}
			}
			cmzn_fielditerator_destroy(&field_iter);
			cmzn_fieldmodule_destroy(&field_module);
		}

		if (recursion_mode == CMZN_STREAMINFORMATION_REGION_RECURSION_MODE_ON)
		{
			// write child regions
			cmzn_region *child_region = cmzn_region_get_first_child(region);
			while (child_region)
			{
				return_code = write_cmzn_region(output_file,
					child_region, group_name, root_region,
					write_elements, write_nodes, write_data,
					write_fields_mode, number_of_field_names, field_names,
					field_names_counter, time, write_criterion, recursion_mode);
				if (!return_code)
				{
					cmzn_region_destroy(&child_region);
					break;
				}
				cmzn_region_reaccess_next_sibling(&child_region);
			}
		}
		if (group)
		{
			cmzn_field_group_destroy(&group);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE, "write_cmzn_region.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* write_cmzn_region */

/*
Global functions
----------------
*/

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(FE_write_criterion)
{
	const char *enumerator_string;

	ENTER(ENUMERATOR_STRING(FE_write_criterion));
	switch (enumerator_value)
	{
		case FE_WRITE_COMPLETE_GROUP:
		{
			enumerator_string = "complete_group";
		} break;
		case FE_WRITE_WITH_ALL_LISTED_FIELDS:
		{
			enumerator_string = "with_all_listed_fields";
		} break;
		case FE_WRITE_WITH_ANY_LISTED_FIELDS:
		{
			enumerator_string = "with_any_listed_fields";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
} /* ENUMERATOR_STRING(FE_write_criterion) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(FE_write_criterion)

PROTOTYPE_ENUMERATOR_STRING_FUNCTION(FE_write_recursion)
{
	const char *enumerator_string;

	ENTER(ENUMERATOR_STRING(FE_write_recursion));
	switch (enumerator_value)
	{
		case FE_WRITE_RECURSIVE:
		{
			enumerator_string = "recursive";
		} break;
		case FE_WRITE_RECURSE_SUBGROUPS:
		{
			enumerator_string = "recurse_subgroups";
		} break;
		case FE_WRITE_NON_RECURSIVE:
		{
			enumerator_string = "non_recursive";
		} break;
		default:
		{
			enumerator_string = (const char *)NULL;
		} break;
	}
	LEAVE;

	return (enumerator_string);
} /* ENUMERATOR_STRING(FE_write_recursion) */

DEFINE_DEFAULT_ENUMERATOR_FUNCTIONS(FE_write_recursion)

/***************************************************************************//**
 * Writes an EX file with supplied root_region at the top level of the file.
 *
 * @param output_file  The file to write region and field data to.
 * @param region  The region to output.
 * @param group  Optional subgroup to output.
 * @param root_region  The region which will become the root region in the EX
 *   file. Need not be the true root of region hierarchy, but must contain
 *   <region>.
 * @param write_elements  If set, write elements and element fields to file.
 * @param write_nodes  If set, write nodes and node fields to file.
 * @param write_data  If set, write data and data fields to file. May only use
 *   if write_elements and write_nodes are 0.
 * @param write_fields_mode  Controls which fields are written to file.
 *   If mode is FE_WRITE_LISTED_FIELDS then:
 *   - Number/list of field_names must be supplied;
 *   - Field names not used in a region are ignored;
 *   - Warnings are given for any field names not used in any output region.
 * @param number_of_field_names  The number of names in the field_names array.
 * @param field_names  Array of field names.
 * @param time  Field values at <time> will be written out if field is time
 *    dependent. If fields are time dependent but <time> is out of range then
 *    the values at nearest time will be written out. If fields are not time
 *    dependent, this parameter is ignored.
 * @param write_criterion  Controls which objects are written. Some modes
 *   limit output to nodes or objects with any or all listed fields defined.
 * @param write_recursion  Controls whether sub-regions and sub-groups are
 *   recursively written.
 */
int write_exregion_to_stream(ostream *output_file,
	struct cmzn_region *region, const char *group_name,
	struct cmzn_region *root_region,
	int write_elements, int write_nodes, int write_data,
	enum FE_write_fields_mode write_fields_mode,
	int number_of_field_names, char **field_names, FE_value time,
	enum FE_write_criterion write_criterion,
	enum cmzn_streaminformation_region_recursion_mode recursion_mode)
{
	int return_code;

	ENTER(write_exregion_to_stream);
	if (output_file && region && root_region &&
		((write_data || write_elements || write_nodes)) &&
		((write_fields_mode != FE_WRITE_LISTED_FIELDS) ||
			((0 < number_of_field_names) && field_names)))
	{
		(*output_file) << "EX Version: 2\n";
		if (cmzn_region_contains_subregion(root_region, region))
		{
			int *field_names_counter = NULL;
			if (0 < number_of_field_names)
			{
				/* count number of times each field name is matched for later warning */
				if (ALLOCATE(field_names_counter, int, number_of_field_names))
				{
					for (int i = 0; i < number_of_field_names; i++)
					{
						field_names_counter[i] = 0;
					}
				}
			}
			return_code = write_cmzn_region(output_file,
				region, group_name, root_region,
				write_elements, write_nodes, write_data,
				write_fields_mode, number_of_field_names, field_names, field_names_counter,
				time, write_criterion, recursion_mode);
			if (field_names_counter)
			{
				if (write_fields_mode == FE_WRITE_LISTED_FIELDS)
				{
					for (int i = 0; i < number_of_field_names; i++)
					{
						if (field_names_counter[i] == 0)
						{
							display_message(WARNING_MESSAGE,
								"No field named '%s' found in any region written to EX file",
								field_names[i]);
						}
					}
				}
				DEALLOCATE(field_names_counter);
			}
			if (!return_code)
			{
				display_message(ERROR_MESSAGE,
					"write_exregion_to_stream.  Error writing region");
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"write_exregion_to_stream.  Region is not within root region");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"write_exregion_to_stream.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* write_exregion_to_stream */

int write_exregion_file_of_name(const char *file_name,
	struct cmzn_region *region, const char *group_name,
	struct cmzn_region *root_region,
	int write_elements, int write_nodes, int write_data,
	enum FE_write_fields_mode write_fields_mode,
	int number_of_field_names, char **field_names, FE_value time,
	enum FE_write_criterion write_criterion,
	enum cmzn_streaminformation_region_recursion_mode recursion_mode)
{
	int return_code;

	if (file_name)
	{
		ofstream output_file;
		output_file.open(file_name, ios::out);
		if (output_file.is_open())
		{
			return_code = write_exregion_to_stream(&output_file, region, group_name, root_region,
				write_elements, write_nodes, write_data,
				write_fields_mode, number_of_field_names, field_names, time,
				write_criterion, recursion_mode);
			output_file.close();
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Could not open for writing exregion file: %s", file_name);
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"write_exregion_file_of_name.  Invalid arguments");
		return_code = 0;
	}

	return (return_code);
} /* write_exregion_file_of_name */

int write_exregion_file_to_memory_block(
	struct cmzn_region *region, const char *group_name,
	struct cmzn_region *root_region, int write_elements,
	int write_nodes, int write_data,
	enum FE_write_fields_mode write_fields_mode,
	int number_of_field_names, char **field_names, FE_value time,
	enum FE_write_criterion write_criterion,
	enum cmzn_streaminformation_region_recursion_mode recursion_mode,
	void **memory_block, unsigned int *memory_block_length)
{
	int return_code;

	ENTER(write_exregion_file_of_name);
	if (memory_block)
	{
		ostringstream stringStream;
		if (stringStream)
		{
			return_code = write_exregion_to_stream(&stringStream, region, group_name, root_region,
				write_elements, write_nodes, write_data,
				write_fields_mode, number_of_field_names, field_names, time,
				write_criterion, recursion_mode);
			string sstring = stringStream.str();
			*memory_block_length = static_cast<unsigned int>(sstring.size());
			*memory_block = duplicate_string(sstring.c_str());
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Could not open for writing exregion into memory");
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"write_exregion_file_of_name.  Invalid arguments");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}
