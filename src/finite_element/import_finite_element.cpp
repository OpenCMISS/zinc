/**
 * FILE : import_finite_element.cpp
 *
 * Functions for importing finite element data from EX file format.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/elementfieldtemplate.h"
#include "opencmiss/zinc/elementtemplate.h"
#include "opencmiss/zinc/fieldgroup.h"
#include "opencmiss/zinc/fieldmodule.h"
#include "opencmiss/zinc/fieldsubobjectgroup.h"
#include "opencmiss/zinc/mesh.h"
#include "opencmiss/zinc/node.h"
#include "opencmiss/zinc/nodeset.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_private.h"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_time.h"
#include "finite_element/import_finite_element.h"
#include "finite_element/node_field_template.hpp"
#include "general/debug.h"
#include "general/math.h"
#include "general/io_stream.h"
#include "general/mystring.h"
#include "general/message.h"
#include "mesh/cmiss_element_private.hpp"

#include <cmath>
#include <ctype.h>
#include <map>
#include <string>
#include <vector>

/*
Module types
------------
*/

namespace {

/** @return  True if zero terminated string s contains at least one contiguous digits
  * optionally padded with any number of leading and/or trailing whitespace.
  * @param n  Optional length of valid string, otherwise must end in zero. */
bool isIntegerString(const char *s, size_t n = 0)
{
	const char *c = s;
	while (isspace(*c))
		++c;
	if (!isdigit(*c))
		return false;
	do
	{
		++c;
	} while (isdigit(*c));
	while (isspace(*c))
		++c;
	if (n > 0)
		return (static_cast<size_t>(c - s) == n);
	return ('\0' == *c);
}

/** Return the next token in the string, skipping initial spaces and
  * ending in white space or specified separator characters.
  * @param s  String to parse. Modified by this function to put null character
  * at end of token. On conclusion, s is advanced past the token, and any
  * spaces plus 0 or 1 separation character.
  * @param sepchars  String containing any separator characters in
  * addition to white space which token stops at.
  * @param nextchar  On return contains the next non-space character after
  * the tokencharacter that was substituted
  * with null character.
  * @return  Pointer to next token in s, or null if zero length. Not to be
  * deallocated. */
const char *nexttoken(char*& s, const char *sepchars, char &nextchar)
{
	while (' ' == *s)
		++s;
	const char *token = s;
	char *e = s;
	while (!(isspace(*e) || (0 != strchr(sepchars, *e))))
		++e;
	nextchar = *e;
	*e = '\0';
	// advance s beyond end of token
	s = e;
	if ('\0' != nextchar)
		++s;
	if (' ' == nextchar)
	{
		while (' ' == *s)
			++s;
		if (0 != strchr(sepchars, *s))
		{
			nextchar = *s;
			++s;
		}
	}
	return token;
}

class KeyValueMap
{
	// pairs of key and value
	std::vector<const char *> strings;
	std::vector<int> stringsUsed;

public:
	~KeyValueMap()
	{
		for (auto iter = this->strings.begin(); iter != this->strings.end(); ++iter)
			DEALLOCATE(*iter);
	}

	/** Adds key value pair to map, if key does not exist already.
	  * On success, takes ownership of strings passed in.
	  * @return  True if succeeded, false if key exists already */
	bool addKeyValue(const char *key, const char *value)
	{
		if ((!key) || (!value) || (this->getKeyIndex(key) >= 0))
			return false;
		const size_t count = this->strings.size();
		this->strings.resize(count + 2);
		this->strings[count] = key;
		this->strings[count + 1] = value;
		this->stringsUsed.push_back(0);
		return true;
	}

	/** @return  Value for key, or 0 if none. Do not deallocate. */
	const char *getKeyValue(const char *key)
	{
		int index = this->getKeyIndex(key);
		if (index < 0)
			return 0;
		this->stringsUsed[index] = 1;
		return this->strings[index*2 + 1];
	}

	bool hasUnusedKeyValues() const
	{
		const size_t count = this->strings.size() / 2;
		for (size_t i = 0; i < count; ++i)
			if (!this->stringsUsed[i])
				return true;
		return false;
	}

	void reportUnusedKeyValues(const char *prefix) const
	{
		const size_t count = this->strings.size() / 2;
		for (size_t i = 0; i < count; ++i)
			if (!this->stringsUsed[i])
			{
				display_message(WARNING_MESSAGE, "%sIgnoring unrecognised key value pair (invalid or from newer version): %s=%s",
					prefix, this->strings[i*2], this->strings[i*2 + 1]);
			}
	}

private:

	/** @return  Index of key, or -1 if not present */
	int getKeyIndex(const char *key)
	{
		const int count = static_cast<int>(this->strings.size()/2);
		for (int i = 0; i < count; ++i)
			if (0 == strcmp(key, this->strings[i*2]))
				return i;
		return -1;
	}

};

} // anonymous namespace

class EXReader
{
	class ScaleFactorSet
	{
	public:
		std::string name;
		int scaleFactorCount;
		int scaleFactorOffset;
		std::vector<cmzn_elementfieldtemplate_scale_factor_type> scaleFactorTypes;
		std::vector<int> scaleFactorIdentifiers;
		std::vector< std::pair<std::string, std::vector<int> > > fieldComponents;  // field name and components using this scale factor set, in order added
		std::vector<FE_element_field_template *> efts;  // EFTs using this scale factor set, converted from field components above
		std::vector<FE_value> values; // cache for reading into

		ScaleFactorSet(const char *nameIn, int scaleFactorCountIn, int scaleFactorOffsetIn) :
			name(nameIn),
			scaleFactorCount(scaleFactorCountIn),
			scaleFactorOffset(scaleFactorOffsetIn),
			scaleFactorTypes(scaleFactorCountIn, CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_PATCH),
			scaleFactorIdentifiers(scaleFactorCountIn, 0),
			values(scaleFactorCountIn)
		{
		}

		/** Add field (as name) and component to list using this scale factor set */
		void addFieldComponent(const char *nameIn, int componentNumberIn)
		{
			const size_t fieldCount = fieldComponents.size();
			for (size_t f = 0; f < fieldCount; ++f)
			{
				std::string &thisName = fieldComponents[f].first;
				if (0 == thisName.compare(nameIn))
				{
					fieldComponents[f].second.push_back(componentNumberIn);
					return;
				}
			}
			std::vector<int> tmpInts(1, componentNumberIn);
			std::pair<std::string, std::vector<int> > thisPair(nameIn, tmpInts);
			this->fieldComponents.push_back(thisPair);
		}

		/** Add eft to list to assign these scale factors to, but avoid repeating */
		void addEFT(FE_element_field_template *eft)
		{
			const size_t count = efts.size();
			for (size_t i = 0; i < count; ++i)
				if (efts[i] == eft)
					return;
			this->efts.push_back(eft);
		}

		bool parseScaleFactorIdentifiers(const char *scaleFactorIdentifiersString)
		{
			if (0 == this->scaleFactorCount)
			{
				display_message(ERROR_MESSAGE, "EX Reader:  Can't have scale factor identifiers if no scale factors");
				return false;
			}
			if (!scaleFactorIdentifiersString)
			{
				return true; // use defaults: all type element_patch, identifier 0
			}
			int index = 0;
			const char *s = scaleFactorIdentifiersString;
			while (true)
			{
				while (isspace(*s))
				{
					++s;
				}
				if (*s == '\0')
				{
					if (index != this->scaleFactorCount)
					{
						display_message(ERROR_MESSAGE, "EX Reader:  Too few scale factor types and identifiers");
						return false;
					}
					break;
				}
				if (index >= this->scaleFactorCount)
				{
					display_message(ERROR_MESSAGE, "EX Reader:  Unexpected text '%s' after scale factor types and identifiers", s);
					return false;
				}
				size_t n = strcspn(s, " (");
				cmzn_elementfieldtemplate_scale_factor_type scaleFactorType = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_INVALID;
				if (0 == strncmp(s, "element_general", n))
				{
					scaleFactorType = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_GENERAL;
				}
				else if (0 == strncmp(s, "element_patch", n))
				{
					scaleFactorType = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_ELEMENT_PATCH;
				}
				else if (0 == strncmp(s, "global_general", n))
				{
					scaleFactorType = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_GENERAL;
				}
				else if (0 == strncmp(s, "global_patch", n))
				{
					scaleFactorType = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_GLOBAL_PATCH;
				}
				else if (0 == strncmp(s, "node_general", n))
				{
					scaleFactorType = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_GENERAL;
				}
				else if (0 == strncmp(s, "node_patch", n))
				{
					scaleFactorType = CMZN_ELEMENTFIELDTEMPLATE_SCALE_FACTOR_TYPE_NODE_PATCH;
				}
				else
				{
					display_message(ERROR_MESSAGE, "EX Reader:  Unknown scale factor type '%.*s'", n, s);
					return false;
				}
				const char *typeString = s;
				s += n;
				while (isspace(*s))
				{
					++s;
				}
				if (*s != '(')
				{
					display_message(ERROR_MESSAGE, "EXReader:  Missing bracket and identifiers (#,#,...) after scale factor type '%.*s'",
						n, typeString);
					return false;
				}
				++s;
				while (true)
				{
					size_t r = strcspn(s, ",)");
					if (!isIntegerString(s, r))
					{
						display_message(ERROR_MESSAGE, "EXReader:  Missing integer identifier after scale factor type '%.*s'",
							n, typeString);
						return false;
					}
					int scaleFactorIdentifier;
					sscanf(s, "%d", &scaleFactorIdentifier);
					this->scaleFactorTypes[index] = scaleFactorType;
					this->scaleFactorIdentifiers[index] = scaleFactorIdentifier;
					++index;
					s += r;
					if (*s != ',')
					{
						if (*s != ')')
						{
							display_message(ERROR_MESSAGE, "EXReader:  Invalid character '%c' in scale factor identifiers for scale factor type %.*s",
								*s, n, typeString);
							return false;
						}
						++s;
						break;
					}
					++s;
					if (index >= this->scaleFactorCount)
					{
						display_message(ERROR_MESSAGE, "EX Reader: Too many scale factor types and identifiers");
						return false;
					}
				}
			}
			return true;
		}

		cmzn_elementfieldtemplate_scale_factor_type getScaleFactorType(int index) const
		{
			return this->scaleFactorTypes[index];
		}

		int getScaleFactorIdentifier(int index) const
		{
			return this->scaleFactorIdentifiers[index];
		}
	};

	int exVersion;
	IO_stream *input_file;
	bool useData;  // True if reading datapoints by default, otherwise nodes
	FE_import_time_index *timeIndex;
	cmzn_region *region;  // accessed
	cmzn_fieldmodule *fieldmodule;  // accessed
	FE_region *fe_region;
	FE_mesh *mesh;
	FE_nodeset *nodeset;
	// current group field and subobject groups being read into
	cmzn_field_group *fieldGroup;  // accessed
	cmzn_nodeset_group *nodesetGroup;  // accessed
	cmzn_mesh_group *meshGroup;  // accessed
	// cache of latest node and element field header:
	FE_element_shape* elementShape;
	FE_node_template *fe_node_template;
	cmzn_elementtemplate *elementtemplate;
	std::vector<FE_field *> headerFields;  // order of fields in header
	bool hasElementValues;  // set to true if any element field has element field values
	std::vector<ScaleFactorSet *> scaleFactorSets;
	char *fileLocation; // cache for storing stream location string for writing with errors. @see getFileLocation

public:
	/** @param timeIndexIn  Optional, specifies time to define field at. */
	EXReader(IO_stream *input_fileIn, FE_import_time_index *timeIndexIn) :
		exVersion(1),
		input_file(input_fileIn),
		useData(false),
		timeIndex(timeIndexIn),
		region(nullptr),
		fieldmodule(nullptr),
		fe_region(nullptr),
		mesh(nullptr),
		nodeset(nullptr),
		fieldGroup(nullptr),
		nodesetGroup(nullptr),
		meshGroup(nullptr),
		elementShape(nullptr),
		fe_node_template(nullptr),
		elementtemplate(nullptr),
		hasElementValues(false),
		fileLocation(nullptr)
	{
	}

	~EXReader()
	{
		this->clearHeaderCache();
		if (this->fileLocation)
			DEALLOCATE(this->fileLocation);
		this->clearGroup();
		this->clearRegion();
	}

	void clearGroup()
	{
		cmzn_field_group_destroy(&(this->fieldGroup));
		this->clearSubelementGroups();
	}

	void clearSubelementGroups()
	{
		cmzn_nodeset_group_destroy(&(this->nodesetGroup));
		cmzn_mesh_group_destroy(&(this->meshGroup));
	}

	void clearRegion()
	{
		cmzn_region_destroy(&(this->region));
		cmzn_fieldmodule_destroy(&(this->fieldmodule));
		this->fe_region = nullptr;
	}

	cmzn_region *getRegion() const
	{
		return this->region;
	}

	bool setRegion(cmzn_region *regionIn)
	{
		if (!regionIn)
			return false;
		this->clearRegion();
		this->region = cmzn_region_access(regionIn);
		this->fieldmodule = cmzn_region_get_fieldmodule(this->region);
		this->fe_region = this->region->get_FE_region();
		this->clearGroup();
		return this->setNodeset(FE_region_find_FE_nodeset_by_field_domain_type(this->fe_region,
			this->useData ? CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS : CMZN_FIELD_DOMAIN_TYPE_NODES));
	}

	cmzn_field_group *getFieldGroup() const
	{
		return this->fieldGroup;
	}

	bool setFieldGroup(cmzn_field_group *fieldGroupIn)
	{
		this->clearGroup();
		if (!fieldGroupIn)
			return true;
		if (Computed_field_get_region(cmzn_field_group_base_cast(fieldGroupIn)) != this->region)
		{
			display_message(ERROR_MESSAGE, "EXReader::setGroup.  Invalid group for region.  %s", this->getFileLocation());
			return false;
		}
		cmzn_field_access(cmzn_field_group_base_cast(fieldGroupIn));
		this->fieldGroup = fieldGroupIn;
		return true;
	}

	bool setFieldGroupOfName(const char *name)
	{
		cmzn_field_id field = cmzn_fieldmodule_find_field_by_name(this->fieldmodule, name);
		cmzn_field_group *group = nullptr;
		if (field)
		{
			group = cmzn_field_cast_group(field);
			cmzn_field_destroy(&field);
			if (!group)
			{
				display_message(ERROR_MESSAGE,
					"EXReader.  Could not create group \'%s\' as name is in use by another field.  %s",
					name, this->getFileLocation());
				return false;
			}
		}
		else
		{
			field = cmzn_fieldmodule_create_field_group(this->fieldmodule);
			cmzn_field_set_managed(field, true);
			cmzn_field_set_name(field, name);
			group = cmzn_field_cast_group(field);
			cmzn_field_destroy(&field);
			if (!group)
			{
				display_message(ERROR_MESSAGE,
					"EXReader.  Could not create group \'%s\'.  %s", name, this->getFileLocation());
				return false;
			}
		}
		bool result = this->setFieldGroup(group);
		cmzn_field_group_destroy(&group);
		return result;
	}

	/** @return  Non-accessed mesh group or nullptr if none */
	cmzn_mesh_group *getMeshGroup()
	{
		if ((!this->meshGroup) && (this->fieldGroup) && (this->mesh))
		{
			cmzn_mesh *extMesh = cmzn_fieldmodule_find_mesh_by_dimension(this->fieldmodule, this->mesh->getDimension());
			cmzn_field_element_group_id fieldElementGroup = cmzn_field_group_get_field_element_group(this->fieldGroup, extMesh);
			if (!fieldElementGroup)
				fieldElementGroup = cmzn_field_group_create_field_element_group(this->fieldGroup, extMesh);
			this->meshGroup = cmzn_field_element_group_get_mesh_group(fieldElementGroup);
			cmzn_field_element_group_destroy(&fieldElementGroup);
			cmzn_mesh_destroy(&extMesh);
		}
		return this->meshGroup;
	}

	/** @return  Non-accessed nodeset group or nullptr if none */
	cmzn_nodeset_group *getNodesetGroup()
	{
		if ((!this->nodesetGroup) && (this->fieldGroup) && (this->nodeset))
		{
			cmzn_nodeset *extNodeset = cmzn_fieldmodule_find_nodeset_by_field_domain_type(this->fieldmodule, this->nodeset->getFieldDomainType());
			cmzn_field_node_group_id fieldNodeGroup = cmzn_field_group_get_field_node_group(this->fieldGroup, extNodeset);
			if (!fieldNodeGroup)
				fieldNodeGroup = cmzn_field_group_create_field_node_group(this->fieldGroup, extNodeset);
			this->nodesetGroup = cmzn_field_node_group_get_nodeset_group(fieldNodeGroup);
			cmzn_field_node_group_destroy(&fieldNodeGroup);
			cmzn_nodeset_destroy(&extNodeset);
		}
		return this->nodesetGroup;
	}

	void setUseDataMetaFlag(bool useDataIn)
	{
		this->useData = useDataIn;
	}

	FE_nodeset *getNodeset() const
	{
		return this->nodeset;
	}

	/** switch to reading nodes into nodeset. Must belong to current FE_region */
	bool setNodeset(FE_nodeset *nodesetIn)
	{
		if ((!nodesetIn) || (nodesetIn->get_FE_region() != this->fe_region))
		{
			display_message(ERROR_MESSAGE, "EXReader::setNodeset.  Invalid nodeset for region.  %s", this->getFileLocation());
			return false;
		}
		this->mesh = nullptr;
		this->nodeset = nodesetIn;
		this->clearHeaderCache();
		this->clearSubelementGroups();
		// set default blank node template for nodeset:
		this->fe_node_template = this->nodeset->create_FE_node_template();
		if (0 == this->fe_node_template)
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Failed to set nodeset.  %s", this->getFileLocation());
			return false;
		}
		return true;
	}

	FE_mesh *getMesh() const
	{
		return this->mesh;
	}

	/** switch to reading elements into mesh. Must belong to current FE_region */
	bool setMesh(FE_mesh *meshIn)
	{
		if ((!meshIn) || (meshIn->get_FE_region() != this->fe_region))
		{
			display_message(ERROR_MESSAGE, "EXReader::setMesh.  Invalid mesh for region");
			return false;
		}
		this->mesh = meshIn;
		this->nodeset = this->mesh->getNodeset();
		this->clearHeaderCache();
		this->clearSubelementGroups();
		return true;
	}

	int readNextNonSpaceChar()
	{
		int next_char;
		do
		{
			next_char = IO_stream_getc(this->input_file);
		} while (next_char == ' ');
		return next_char;
	}

	const char *getFileLocation()
	{
		if (this->fileLocation)
			DEALLOCATE(this->fileLocation);
		this->fileLocation = IO_stream_get_location_string(this->input_file);
		return this->fileLocation;
	}

	bool readEXVersion();
	bool readCommentOrDirective();
	bool readFieldValues();
	bool readNodeHeader();
	cmzn_node *readNode();
	bool readElementShape();
	bool readElementHeader();
	bool readElementIdentifier(DsLabelIdentifier &elementIdentifier);
	bool readElementFieldComponentValues(DsLabelIndex elementIndex, FE_field *field, int componentNumber);
	cmzn_element *readElement();

private:

	void clearHeaderCache(bool clearElementShape = true)
	{
		cmzn::Deaccess(this->fe_node_template);
		if (clearElementShape && (this->elementShape))
			DEACCESS(FE_element_shape)(&(this->elementShape));
		cmzn_elementtemplate::deaccess(this->elementtemplate);
		const size_t sfCount = this->scaleFactorSets.size();
		for (size_t s = 0; s < sfCount; ++s)
			delete this->scaleFactorSets[s];
		this->scaleFactorSets.clear();
		this->headerFields.clear();
		this->hasElementValues = false;
	}

	/*** @return  Pointer to scale factor set with name and details, or 0 if failed */
	ScaleFactorSet *createScaleFactorSet(const char *nameIn, int scaleFactorCountIn, int scaleFactorOffsetIn,
		const char *scaleFactorIdentifiersString)
	{
		if ((!nameIn) || (scaleFactorCountIn < 1) || (scaleFactorOffsetIn < 0))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Missing scale factor set name.  %s", this->getFileLocation());
			return 0;
		}
		if (0 != this->findScaleFactorSet(nameIn))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Scale factor set %s already defined.  %s", nameIn, this->getFileLocation());
			return 0;
		}
		ScaleFactorSet *sfSet = new ScaleFactorSet(nameIn, scaleFactorCountIn, scaleFactorOffsetIn);
		if (!sfSet)
		{
			return 0;
		}
		if (!sfSet->parseScaleFactorIdentifiers(scaleFactorIdentifiersString))
		{
			delete sfSet;
			return 0;
		}
		this->scaleFactorSets.push_back(sfSet);
		return sfSet;
	}

	/*** @return  Pointer to scale factor set with name, or 0 if not found */
	ScaleFactorSet *findScaleFactorSet(const char *nameIn)
	{
		if (!nameIn)
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Missing scale factor set name.  %s", this->getFileLocation());
			return 0;
		}
		const size_t sfCount = this->scaleFactorSets.size();
		for (size_t s = 0; s < sfCount; ++s)
		{
			if (0 == this->scaleFactorSets[s]->name.compare(nameIn))
				return this->scaleFactorSets[s];
		}
		return 0;
	}

	bool readBlankToEndOfLine();
	bool readKeyValueMap(KeyValueMap& keyValueMap, int initialSeparator = 0);
	bool readElementXiValue(FE_field *field, cmzn_element* &element, FE_value *xi);
	bool checkConsumeNextChar(char testChar);
	char *readString(const char *stringDelimitersFormat = "[^ ,;=\n\r\t]");
	FE_field *readField();
	bool readNodeValueLabelsVersions(FE_node_field_template& nft);
	bool readNodeHeaderField();
	bool createElementtemplate();
	struct FE_basis *readBasis();
	bool readElementHeaderField();
};

/** Read text to end of line.
  * @return  True if rest of line contains only spaces or tabs, false if unexpected text. */
bool EXReader::readBlankToEndOfLine()
{
	char *rest_of_line;
	if (!IO_stream_read_string(input_file, "[^\n\r]", &rest_of_line))
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Failed to read rest of line.  %s", this->getFileLocation());
		return false;
	}
	bool result = true;
	const char *c = rest_of_line;
	while (*c != '\0')
	{
		if ((*c != ' ') && (*c != '\t'))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Unexpected text '%s' on line.  %s", rest_of_line, this->getFileLocation());
			result = false;
			break;
		}
	}
	DEALLOCATE(rest_of_line);
	// skip end of line characters and following white space
	IO_stream_scan(this->input_file, " ");
	return result;
}

/** @return  True if next char is testChar (and read the character) or false if
  * node (and do not read the character) */
bool EXReader::readEXVersion()
{
	if (this->exVersion > 1)
	{
		display_message(ERROR_MESSAGE, "EX Reader.  EX Version has already been specified.  %s", this->getFileLocation());
		return false;
	}
	if (this->fe_region)
	{
		display_message(ERROR_MESSAGE, "EX Reader.  EX Version number must be first token.  %s", this->getFileLocation());
		return false;
	}
	int versionNumber;
	if (1 != IO_stream_scan(this->input_file, " Version :%d", &versionNumber)) // "EX" has been read before getting here
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Error reading EX Version: number.  %s", this->getFileLocation());
		return false;
	}
	if (versionNumber < 2)
	{
		display_message(ERROR_MESSAGE, "EX Reader.  EX Version number must be >= 2 (it is not specified for version 1).  %s", this->getFileLocation());
		return false;
	}
	if (versionNumber > 2)
	{
		display_message(ERROR_MESSAGE, "EX Reader.  This library cannot read EX Version %d, only version 2 may be specified.  %s",
			versionNumber, this->getFileLocation());
		return false;
	}
	this->exVersion = versionNumber;
	return true;
}

/**
 * Reads to the end of the line extracting any comma-separated key=value pairs.
 * End of line characters and subsequent whitespace are consumed.
 * It's an error if any other text is present.
 * Keys may contain any characters including spaces, but leading and trailing
 * whitespace is trimmed. This must be followed by '=' then a string read by
 * EXReader::readString.
 * @param keyValueMap  The key value map structure to fill. Expected to be empty.
 * @param initialSeparator  Optional initial separator required before key value pairs.
 * If present, a key=value pair must follow (as when there is a , after a pair). If
 * not present, the rest of the line may be blank only.
 * @return  True on success, false on failure.
 */
bool EXReader::readKeyValueMap(KeyValueMap& keyValueMap, int initialSeparator)
{
	int separator = initialSeparator;
	int next_char;
	while (true)
	{
		if (0 != separator)
		{
			next_char = this->readNextNonSpaceChar();
			if (next_char != separator)
			{
				if (((int)'\n' == next_char) || ((int)'\r' == next_char))
					break;
				char *rest_of_line;
				IO_stream_read_string(input_file, "[^\n\r]", &rest_of_line);
				display_message(ERROR_MESSAGE, "EX Reader.  Unexpected text '%c%s' where only '%c key=value[, key=value[, ...]]' allowed.  %s",
					(char)next_char, rest_of_line, (char)separator, this->getFileLocation());
				DEALLOCATE(rest_of_line);
				return false;
			}
		}
		char *key = 0;
		if (!IO_stream_read_string(this->input_file, "[^=\n\r]", &key))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Failed to read key=value key.  %s", this->getFileLocation());
			return false;
		}
		trim_string_in_place(key);
		next_char = IO_stream_getc(this->input_file);
		if ((int)'=' != next_char)
		{
			if ((0 == separator) && (strlen(key) == 0))
			{
				DEALLOCATE(key);
				break;
			}
			display_message(ERROR_MESSAGE, "EX Reader.  Unexpected text '%s' where only 'key=value[, key=value[, ...]]' allowed.  %s",
				key, this->getFileLocation());
			DEALLOCATE(key);
			return false;
		}
		if (strlen(key) == 0)
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Invalid key=value key '%s'.  %s", key, this->getFileLocation());
			DEALLOCATE(key);
			return false;
		}
		// note string value can be on next line
		char *value = this->readString();
		if (!value)
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Missing value for key=value pair.  %s", this->getFileLocation());
			DEALLOCATE(key);
			return false;
		}
		if (!keyValueMap.addKeyValue(key, value))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Duplicate key in key=value list.  %s", this->getFileLocation());
			DEALLOCATE(key);
			DEALLOCATE(value);
			return false;
		}
		separator = (int)',';
	}
	// consume end of line characters
	IO_stream_scan(this->input_file, "[\n\r]");
	return true;
}

/**
 * Reads an element:xi location from the stream. From version 2 the format is:
 * ELEMENT_NUMBER xi1 xi2... xiDIMENSION
 * and the host mesh must have been specified in the field header.
 * The format for earlier versions is:
 * [REGION_PATH] E<lement>/F<ace>/L<ine> ELEMENT_NUMBER DIMENSION xi1 xi2... xiDIMENSION
 * The optional region path is in fact ignored; only a mesh from the current
 * region can be used as the host and this determined from the first element
 * if not specified in the header.
 * @param element  On successful return, and non-accessed element or 0 if none.
 * @param xi   Must have enough space for the dimension of the host mesh, or
 * up to MAXIMUM_ELEMENT_XI_DIMENSIONS FE_values in the earlier format.
 */
bool EXReader::readElementXiValue(FE_field *field, cmzn_element* &element, FE_value *xi)
{
	if (!(field && xi))
	{
		display_message(ERROR_MESSAGE, "EXReader::readElementXiValue.  Invalid argument(s)");
		return false;
	}
	// const_cast is dirty, but mesh is constant as far as field is concerned,
	// however this function can create blank elements in mesh
	FE_mesh *hostMesh = field->getElementXiHostMesh();
	DsLabelIdentifier elementIdentifier;
	if (this->exVersion >= 2)
	{
		if (1 != IO_stream_scan(this->input_file, " %d", &elementIdentifier))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Missing element number in element:xi value.  %s", this->getFileLocation());
			return false;
		}
	}
	else
	{
		bool success = true;
		// determine the region path, element type and element number. First read
		// two strings and determine if the second is a number, in which case the
		// region path is omitted
		char *whitespace_string = 0;
		char *first_string = 0;
		char *separator_string = 0;
		char *second_string = 0;
		IO_stream_read_string(this->input_file, "[ \n\r\t]", &whitespace_string);
		if (IO_stream_read_string(this->input_file, "[^ \n\r\t]", &first_string) &&
			IO_stream_read_string(this->input_file, "[ \n\r\t]", &separator_string) &&
			IO_stream_read_string(this->input_file, "[^ \n\r\t]", &second_string))
		{
			char *element_type_string = 0;
			// first determine the element_number, which is in the second_string
			// if the region path has been omitted, otherwise next in the file
			if (1 == sscanf(second_string, " %d", &elementIdentifier))
			{
				element_type_string = first_string;
			}
			else if (1 == IO_stream_scan(this->input_file, " %d", &elementIdentifier))
			{
				display_message(WARNING_MESSAGE, "EX Reader.  Ignoring region path in legacy element:xi field %s values", get_FE_field_name(field));
				element_type_string = second_string;
			}
			else
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Missing element number in legacy element:xi value.  %s", this->getFileLocation());
				success = false;
			}
			if (element_type_string)
			{
				// element type is redundant since sufficient to read dimension which follows element number
				if (!(fuzzy_string_compare(element_type_string, "element")
					|| fuzzy_string_compare(element_type_string, "face")
					|| fuzzy_string_compare(element_type_string, "line")))
				{
					display_message(ERROR_MESSAGE, "Unknown legacy element type %s for element_xi value.  %s",
						element_type_string, this->getFileLocation());
					success = false;
				}
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Missing legacy region path, element type or number in element:xi value.  %s", this->getFileLocation());
			success = false;
		}
		DEALLOCATE(second_string);
		DEALLOCATE(separator_string);
		DEALLOCATE(first_string);
		DEALLOCATE(whitespace_string);
		if (!success)
			return false;
		int dimension;
		if ((1 != IO_stream_scan(this->input_file, " %d", &dimension)) || (dimension < 1) || (dimension > MAXIMUM_ELEMENT_XI_DIMENSIONS))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Invalid or missing element dimension in legacy element:xi value.  %s", this->getFileLocation());
			return false;
		}
		if (hostMesh)
		{
			if (dimension != hostMesh->getDimension())
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Element dimension does not match host mesh; "
					"beware element:xi fields are now limited to storing locations in one mesh, set from first element in legacy EX format.  %s",
					this->getFileLocation());
				return false;
			}
		}
		else
		{
			// host mesh is set from first element
			hostMesh = FE_region_find_FE_mesh_by_dimension(this->fe_region, dimension);
			if (!field->setElementXiHostMesh(hostMesh))
				return false;
		}
	}
	if (!hostMesh)
	{
		display_message(ERROR_MESSAGE, "EXReader::readElementXiValue.  Missing host mesh");
		return false;
	}
	if (elementIdentifier < 0)
	{
		element = 0;
	}
	else
	{
		element = hostMesh->get_or_create_FE_element_with_identifier(elementIdentifier, static_cast<FE_element_shape *>(0));
		if (!element)
		{
			display_message(ERROR_MESSAGE, "EXReader::readElementXiValue.  Failed to get or create element.  %s", this->getFileLocation());
			return false;
		}
		// must remove access count
		cmzn_element *tmpElement = element;
		cmzn_element::deaccess(tmpElement);
	}
	for (int d = 0; d < hostMesh->getDimension(); ++d)
	{
		if (1 != IO_stream_scan(this->input_file, FE_VALUE_INPUT_STRING, &(xi[d])))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Missing xi value(s).  %s", this->getFileLocation());
			return false;
		}
		if (!finite(xi[d]))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Infinity or NAN xi coordinates read.  %s", this->getFileLocation());
			return false;
		}
	}
	return true;
}

/** @param testChar  A non white-space character.
  * @return  True if the next character is testChar and advance stream past it.
  * Otherwise return false with no change in stream location. */
bool EXReader::checkConsumeNextChar(char testChar)
{
	if (IO_stream_peekc(this->input_file) == static_cast<int>(testChar))
	{
		IO_stream_getc(this->input_file);
		return true;
	}
	return false;
}

/** 
 * Read a string from the stream after skipping initial whitespace.
 * Works in two modes:
 * 1. If the first non whitespace character is a single or double quote, reads
 * characters until the final matching quote; quotes may be in the string
 * if preceded by the escape character \ (backslash), and a backslash itself
 * must be escaped. For historic compatibility with make_valid_token, '\$' is
 * read as '$' (a perl hangover from Cmgui). Can extend over multiple lines.
 * 2. Non-quoted string is read while characters are valid according to the
  * stringDelimitersFormat with default [^ ,;=\n\r\t], i.e. end on whitespace
  * EOF, ',', ';', '='. Caller may supply a different stringDelimitersFormat.
 * @see make_valid_token
 * @return  Allocated string or 0 if failed. */
char *EXReader::readString(const char *stringDelimitersFormat)
{
	char *theString = 0;
	char *whitespaceString = 0;
	IO_stream_read_string(this->input_file, "[ \n\r\t]", &whitespaceString);
	DEALLOCATE(whitespaceString);
	const char quoteChar =
		(this->checkConsumeNextChar('\"')) ? '\"' :
		(this->checkConsumeNextChar('\'')) ? '\'' :
		0;
	if (!quoteChar)
	{
		if (!(IO_stream_read_string(this->input_file, stringDelimitersFormat,
			&theString) && (theString)))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Failed to read unquoted string.  %s", this->getFileLocation());
		}
		else if (strlen(theString) == 0)
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Missing string.  %s", this->getFileLocation());
			DEALLOCATE(theString);
		}
	}
	else
	{
		const char *format = (quoteChar == '\"') ? "[^\"]" : "[^']";
		size_t length = 0;
		while (true)
		{
			char *block = 0;
			if (!(IO_stream_read_string(this->input_file, format, &block) && (block)))
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Failed to read part of quoted string.  %s", this->getFileLocation());
				if (theString)
					DEALLOCATE(theString);
				break;
			}
			int blockLength = static_cast<int>(strlen(block));
			if (blockLength > 0)
			{
				// process escaped characters
				char *dest = block;
				for (int i = 0; i < blockLength; ++i)
				{
					if (block[i] == '\\')
					{
						++i;
						if (i < blockLength)
						{
							if (block[i] == 'n')
								*dest = '\n';
							else if (block[i] == 't')
								*dest = '\t';
							else if (block[i] == 'r')
								*dest = '\r';
							else
								*dest = block[i];
						}
						else
						{
							// last character is escape, so must be escaping quote_char, or end of file
							const int this_char = IO_stream_getc(this->input_file);
							if (this_char == (int)quoteChar)
							{
								*dest = quoteChar;
							}
							else
							{
								display_message(ERROR_MESSAGE, "EX Reader.  End of file after escape character in string.  %s", this->getFileLocation());
								DEALLOCATE(block);
								break;
							}
						}
					}
					else
					{
						*dest = block[i];
					}
					++dest;
				}
				if (!block)
				{
					if (theString)
						DEALLOCATE(theString);
					break;
				}
				*dest = '\0';
				char *tmp;
				REALLOCATE(tmp, theString, char, length + strlen(block) + 1);
				if (tmp)
				{
					theString = tmp;
					strcpy(theString + length, block);
				}
				else
				{
					if (theString)
						DEALLOCATE(theString);
				}
			}
			DEALLOCATE(block);
			if (theString == 0)
				break;
			length = strlen(theString);
			if ((blockLength == 0) || (theString[length - 1] != quoteChar))
				break;
		}
		if (theString)
		{
			// now get the matching end quote, and check only one
			if (this->checkConsumeNextChar(quoteChar))
			{
				const int nextChar = IO_stream_peekc(this->input_file);
				if (!(isspace(nextChar) || (EOF == nextChar)))
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Require whitespace or EOF after end quote on string.  %s", this->getFileLocation());
					DEALLOCATE(theString);
				}
			}
			else
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Missing end quote on string.  %s", this->getFileLocation());
				DEALLOCATE(theString);
			}
		}
	}
	return theString;
}

/**
 * Read a comment or one of the following directives (assume all started with !):
 * #nodeset NAME[, key=value]
 * #mesh NAME, dimension=#[, face mesh=NAME][, nodeset=NAME][, key=value]
 * where square brackets enclose optional or dimension-dependent data.
 * Unused additional comma separated key=value pairs are warned about.
 * Comment lines are ignored.
 * @return  True on success, false if error in directive.
 */
bool EXReader::readCommentOrDirective()
{
	char test_string[5];
	char nextChar;
	const int input_result = IO_stream_scan(this->input_file, "%c", &nextChar);
	if (1 != input_result)
		return true; // last line
	const bool hasDirectiveChar = (nextChar == '#');
	const bool nodesetDirective = hasDirectiveChar && (1 == IO_stream_scan(this->input_file, "nodese%1[t] ", test_string));
	const bool meshDirective = hasDirectiveChar && (!nodesetDirective) && (1 == IO_stream_scan(this->input_file, "mes%1[h] ", test_string));
	if (!(nodesetDirective || meshDirective))
	{
		// comment: read and ignore to end of line, if not already there
		if ((nextChar != '\n') && (nextChar != '\r'))
		{
			char *rest_of_line;
			IO_stream_read_string(this->input_file, "[^\n\r]", &rest_of_line);
			DEALLOCATE(rest_of_line);
		}
		return true;
	}
	if (!this->fe_region)
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Region/Group must be set before %s directive.  %s",
			nodesetDirective ? "nodeset" : "mesh", this->getFileLocation());
		return false;
	}
	char *name = this->readString();
	if (!name)
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Missing or invalid %s name.  %s",
			nodesetDirective ? "nodeset" : "mesh", this->getFileLocation());
		return false;
	}
	std::string domainName(name);
	DEALLOCATE(name);
	KeyValueMap keyValueMap;
	if (!this->readKeyValueMap(keyValueMap, (int)','))
		return false;
	if (nodesetDirective)
	{
		cmzn_field_domain_type domainType = this->useData ? CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS : CMZN_FIELD_DOMAIN_TYPE_NODES;
		if (0 == domainName.compare("nodes"))
			domainType = CMZN_FIELD_DOMAIN_TYPE_NODES;
		else if (0 == domainName.compare("datapoints"))
			domainType = CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS;
		else
			display_message(WARNING_MESSAGE, "EX Reader.  Can't handle arbitrary nodeset name '%s' in directive, assuming default %s.  %s",
				domainName.c_str(), this->useData ? "datapoints" : "nodes", this->getFileLocation());
		FE_nodeset *nodeset = FE_region_find_FE_nodeset_by_field_domain_type(this->fe_region, domainType);
		if (!this->setNodeset(nodeset))
			return false;
	}
	else if (meshDirective)
	{
		const char *dimensionString = keyValueMap.getKeyValue("dimension");
		const int dimension = (dimensionString) ? atoi(dimensionString) : 0;
		if ((dimension < 1) || (dimension > MAXIMUM_ELEMENT_XI_DIMENSIONS))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Missing or invalid dimension in mesh directive.  %s", this->getFileLocation());
			return false;
		}
		FE_mesh *mesh = FE_region_find_FE_mesh_by_dimension(this->fe_region, dimension);
		if (!this->setMesh(mesh))
			return false;
		// currently mesh, nodeset and face mesh names are read but can't be arbitrary so read and warn if not followed
		if (0 != domainName.compare(this->mesh->getName()))
		{
			display_message(WARNING_MESSAGE, "EX Reader.  Can't handle arbitrary mesh name '%s' in directive, assuming %s (matching dimension).  %s",
				domainName.c_str(), this->mesh->getName(), this->getFileLocation());
		}
		const char *faceMeshName = keyValueMap.getKeyValue("face mesh");
		if (faceMeshName)
		{
			FE_mesh *faceMesh = this->mesh->getFaceMesh();
			if (faceMesh)
			{
				if (0 != strcmp(faceMesh->getName(), faceMeshName))
					display_message(WARNING_MESSAGE, "EX Reader.  Can't handle arbitrary face mesh name '%s' in directive, assuming default %s.  %s",
						domainName.c_str(), faceMesh->getName(), this->getFileLocation());
			}
			else
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Mesh of chosen dimension has no face mesh.  %s", this->mesh->getName(), this->getFileLocation());
				return false;
			}
		}
		const char *nodesetName = keyValueMap.getKeyValue("nodeset");
		if (nodesetName)
		{
			if (this->nodeset)
			{
				if (0 != strcmp(this->nodeset->getName(), nodesetName))
					display_message(WARNING_MESSAGE, "EX Reader.  Can't handle arbitrary nodeset name '%s' in directive, assuming default %s.  %s",
						domainName.c_str(), this->nodeset->getName(), this->getFileLocation());
			}
		}
	}
	if (keyValueMap.hasUnusedKeyValues())
	{
		std::string prefix(nodesetDirective ? "EX Reader.  nodeset directive: " : "EX Reader.  mesh directive: ");
		keyValueMap.reportUnusedKeyValues(prefix.c_str());
	}
	return true;
}

/**
 * Reads a field from its description in stream. Note that the same format is
 * used for node and element field headers.
 * The returned field will be "of" <fe_region>, but not in it. This means it has
 * access to information such as FE_time that is private to <fe_region> and can be
 * simply merged into it using FE_region_merge_FE_field.
 * This approach is used because component names are set later and differently
 * for node and element fields.
 * @return  Accessed and unmerged field, or 0 if failed.
 */
FE_field *EXReader::readField()
{
	enum CM_field_type cm_field_type;
	enum FE_field_type fe_field_type = UNKNOWN_FE_FIELD;
	enum Value_type value_type = UNKNOWN_VALUE;
	FE_value focus;
	FE_mesh *elementXiHostMesh = nullptr;
	int number_of_components, number_of_indexed_values;
	struct Coordinate_system coordinate_system;
	FE_field *field = 0;
	FE_field *indexer_field = 0;
	int return_code = 1;
	char *field_name = 0;
	/* read the field information */
	IO_stream_scan(this->input_file, " %*d) ");
	/* read the field name */
	if (return_code)
	{
		if (IO_stream_read_string(this->input_file, "[^,]", &field_name))
		{
			IO_stream_scan(this->input_file, ", ");
			/* remove trailing blanks off field name */
			size_t i = strlen(field_name);
			while ((0 < i) && (isspace(field_name[i - 1])))
				--i;
			field_name[i] = '\0';
			if (0 == i)
			{
				display_message(ERROR_MESSAGE, "EX Reader.  No field name.  %s", this->getFileLocation());
				return_code = 0;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Missing field name.  %s", this->getFileLocation());
			return_code = 0;
		}
	}
	char *next_block = 0;
	if (return_code)
	{
		/* next string required for CM_field_type, below */
		if (!IO_stream_read_string(this->input_file, "[^,]", &next_block))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Field %s missing CM field type.  %s", field_name, this->getFileLocation());
			return_code = 0;
		}
		IO_stream_scan(this->input_file, ", ");
	}
	/* read the CM_field_type */
	if (return_code)
	{
		if (!STRING_TO_ENUMERATOR(CM_field_type)(next_block, &cm_field_type))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Field %s has unknown CM field type '%s'.  %s",
				field_name, next_block, this->getFileLocation());
			return_code = 0;
		}
		DEALLOCATE(next_block);
	}
	/* read the FE_field_information */
	if (return_code)
	{
		/* next string required for value_type, below */
		if (!IO_stream_read_string(this->input_file, "[^,]", &next_block))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Field %s missing value type information.  %s", field_name, this->getFileLocation());
			return_code = 0;
		}
		IO_stream_scan(this->input_file, ", ");
	}
	/* read the optional modifier: constant|indexed */
	if (return_code)
	{
		if (fuzzy_string_compare_same_length(next_block, "constant"))
		{
			fe_field_type = CONSTANT_FE_FIELD;
		}
		else if (fuzzy_string_compare_same_length(next_block, "indexed"))
		{
			fe_field_type = INDEXED_FE_FIELD;
			DEALLOCATE(next_block);
			if ((EOF != IO_stream_scan(this->input_file, " Index_field = ")) &&
				IO_stream_read_string(this->input_file, "[^,]", &next_block))
			{
				indexer_field = FE_region_get_FE_field_from_name(this->fe_region, next_block);
				if (!indexer_field)
				{
					/* create and merge an appropriate indexer field */
					FE_field *temp_indexer_field = FE_field::create(next_block, this->fe_region);
					if (!(set_FE_field_number_of_components(temp_indexer_field, 1) &&
						set_FE_field_value_type(temp_indexer_field, INT_VALUE) &&
						(indexer_field = FE_region_merge_FE_field(this->fe_region,
							temp_indexer_field))))
					{
						return_code = 0;
					}
					FE_field::deaccess(&temp_indexer_field);
				}
				if (return_code)
				{
					if (!((1 == IO_stream_scan(this->input_file, ", #Values=%d",
						&number_of_indexed_values)) && (0 < number_of_indexed_values)))
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Field %s missing number of indexed values.  %s",
							field_name, this->getFileLocation());
						return_code = 0;
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"EX Reader.  Field %s missing indexing information.  %s", field_name,
					this->getFileLocation());
				return_code = 0;
			}
			IO_stream_scan(this->input_file, ", ");
		}
		else
		{
			fe_field_type = GENERAL_FE_FIELD;
		}
		if (GENERAL_FE_FIELD != fe_field_type)
		{
			DEALLOCATE(next_block);
			if (return_code)
			{
				/* next string required for coordinate system or value_type */
				return_code = IO_stream_read_string(this->input_file, "[^,]", &next_block);
				IO_stream_scan(this->input_file, ", ");
			}
		}
	}
	/* read the coordinate system (optional) */
	coordinate_system.type = NOT_APPLICABLE;
	if (return_code)
	{
		if (fuzzy_string_compare_same_length(next_block,
			"rectangular cartesian"))
		{
			coordinate_system.type = RECTANGULAR_CARTESIAN;
		}
		else if (fuzzy_string_compare_same_length(next_block,
			"cylindrical polar"))
		{
			coordinate_system.type = CYLINDRICAL_POLAR;
		}
		else if (fuzzy_string_compare_same_length(next_block,
			"spherical polar"))
		{
			coordinate_system.type = SPHERICAL_POLAR;
		}
		else if (fuzzy_string_compare_same_length(next_block,
			"prolate spheroidal"))
		{
			coordinate_system.type = PROLATE_SPHEROIDAL;
			IO_stream_scan(this->input_file, " focus=");
			if ((1 != IO_stream_scan(this->input_file, FE_VALUE_INPUT_STRING, &focus)) ||
				(!finite(focus)))
			{
				focus = 1.0;
			}
			coordinate_system.parameters.focus = focus;
			IO_stream_scan(this->input_file, " ,");
		}
		else if (fuzzy_string_compare_same_length(next_block,
			"oblate spheroidal"))
		{
			coordinate_system.type = OBLATE_SPHEROIDAL;
			IO_stream_scan(this->input_file," focus=");
			if ((1 != IO_stream_scan(this->input_file,FE_VALUE_INPUT_STRING, &focus)) ||
				(!finite(focus)))
			{
				focus = 1.0;
			}
			coordinate_system.parameters.focus = focus;
			IO_stream_scan(this->input_file, " ,");
		}
		else if (fuzzy_string_compare_same_length(next_block,
			"fibre"))
		{
			coordinate_system.type = FIBRE;
			value_type = FE_VALUE_VALUE;
		}
		if (NOT_APPLICABLE != coordinate_system.type)
		{
			DEALLOCATE(next_block);
			if (return_code)
			{
				/* next string required for value_type, below */
				return_code = IO_stream_read_string(this->input_file, "[^,\n\r]", &next_block);
				IO_stream_scan(this->input_file, ", ");
			}
		}
	}
	/* read the value_type */
	if (return_code)
	{
		value_type = Value_type_from_string(next_block);
		if (UNKNOWN_VALUE == value_type)
		{
			// before version 2 you could have coordinate system and not value type (real assumed). Now must have value type
			if ((this->exVersion < 2) && (coordinate_system.type != NOT_APPLICABLE))
			{
				/* for backwards compatibility default to FE_VALUE_VALUE if
					coordinate system specified */
				value_type = FE_VALUE_VALUE;
			}
			else
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Field %s has unknown value_type %s.  %s", field_name, next_block, this->getFileLocation());
				return_code = 0;
			}
		}
		else
		{
			DEALLOCATE(next_block);
			/* next string required for #Components, below */
			return_code = IO_stream_read_string(this->input_file,"[^,\n\r]", &next_block);
		}
	}
	// #Components=N is mandatory
	if (return_code)
	{
		if (!((next_block)
			&& (1 == sscanf(next_block, " #Components=%d", &number_of_components))
			&& (0 < number_of_components)))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Field %s missing #Components.  %s", field_name, this->getFileLocation());
			return_code = 0;
		}
	}
	if (next_block)
		DEALLOCATE(next_block);
	if (return_code)
	{
		// read arbitrary comma separated key=value pairs for all additional options (including future)
		// must be preceded by initialSeparator
		KeyValueMap keyValueMap;
		const int initialSeparator = ((this->exVersion < 2) && (ELEMENT_XI_VALUE == value_type)) ? (int)';' : ',';
		if (!this->readKeyValueMap(keyValueMap, initialSeparator))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Failed to read additional key=value parameters for field %s.  %s",
				field_name, this->getFileLocation());
			return_code = 0;
		}
		else
		{
			if (ELEMENT_XI_VALUE == value_type)
			{
				// before EX version 2, optional: mesh dimension=N (if not specified, determine from first embedded element location in old format)
				// from EX version 2, require: host mesh=name, host mesh dimension=N
				const char *dimensionString = keyValueMap.getKeyValue((this->exVersion < 2) ? "mesh dimension" : "host mesh dimension");
				if (dimensionString)
				{
					const int dimension = atoi(dimensionString);
					elementXiHostMesh = FE_region_find_FE_mesh_by_dimension(this->fe_region, dimension);
					if (!elementXiHostMesh)
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Invalid host mesh dimension %d for element:xi valued field %s.  %s",
							dimension, field_name, this->getFileLocation());
						return_code = 0;
					}
				}
				else if (this->exVersion >= 2)
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Missing 'host mesh dimension=N' for element:xi valued field %s.  %s",
						field_name, this->getFileLocation());
					return_code = 0;
				}
				if (this->exVersion >= 2)
				{
					const char *hostMeshName = keyValueMap.getKeyValue("host mesh");
					if (!hostMeshName)
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Missing 'host mesh=~' for element:xi valued field %s'.  %s",
							field_name, this->getFileLocation());
						return_code = 0;
					}
					else if ((elementXiHostMesh) && (0 != strcmp(hostMeshName, elementXiHostMesh->getName())))
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Cannot find host mesh %s specified for element:xi valued field %s.  %s",
							hostMeshName, field_name, this->getFileLocation());
						return_code = 0;
					}
				}
			}
			if (keyValueMap.hasUnusedKeyValues())
			{
				std::string prefix("EX Reader.  Field ");
				prefix += field_name;
				prefix += " header: ";
				keyValueMap.reportUnusedKeyValues(prefix.c_str());
			}
		}
	}
	if (return_code)
	{
		/* create the field */
		field = FE_field::create(field_name, this->fe_region);
		if (!set_FE_field_value_type(field, value_type))
		{
			return_code = 0;
		}
		if (elementXiHostMesh)
		{
			if (!field->setElementXiHostMesh(elementXiHostMesh))
			{
				return_code = 0;
			}
		}
		if (!set_FE_field_number_of_components(field, number_of_components))
		{
			return_code = 0;
		}
		if (!(((CONSTANT_FE_FIELD != fe_field_type) ||
			set_FE_field_type_constant(field)) &&
			((GENERAL_FE_FIELD != fe_field_type) ||
				set_FE_field_type_general(field)) &&
			((INDEXED_FE_FIELD != fe_field_type) ||
				set_FE_field_type_indexed(field, indexer_field,
					number_of_indexed_values))))
		{
			return_code = 0;
		}
		field->set_CM_field_type(cm_field_type);
		field->setCoordinateSystem(coordinate_system);
		if (!return_code)
		{
			display_message(ERROR_MESSAGE,
				"EXReader::readField.  Could not create field '%s'", field_name);
			FE_field::deaccess(&field);
		}
	}
	DEALLOCATE(field_name);
	return (field);
}

/** reads token "Values: followed by field constants, if any constant or indexed fields */
bool EXReader::readFieldValues()
{
	char test_string[5];
	if (1 != IO_stream_scan(this->input_file, "alues %1[:] ", test_string)) // "V" has already been read
	{
		display_message(WARNING_MESSAGE, "EX Reader.  Truncated read of Values: token.  %s", this->getFileLocation());
		return false;
	}
	if (!((this->fe_node_template || this->elementtemplate)))
	{
		display_message(ERROR_MESSAGE, "EXReader.  Must have a current node or element template to read field values.  %s", this->getFileLocation());
		return false;
	}
	const size_t fieldCount = this->headerFields.size();
	for (size_t f = 0; f < fieldCount; ++f)
	{
		FE_field *field = this->headerFields[f];
		const int number_of_values = field->getNumberOfValues();
		if (0 < number_of_values)
		{
			Value_type value_type = get_FE_field_value_type(field);
			switch (value_type)
			{
				case FE_VALUE_VALUE:
				{
					FE_value value;
					for (int k = 0; k < number_of_values; ++k)
					{
						if (!((1 == IO_stream_scan(this->input_file, FE_VALUE_INPUT_STRING, &value))
							&& finite(value)
							&& set_FE_field_FE_value_value(field, k, value)))
						{
							display_message(ERROR_MESSAGE, "EX Reader.  Error reading real field value.  %s", this->getFileLocation());
							return false;
						}
					}
				} break;
				case INT_VALUE:
				{
					int value;
					for (int k = 0; k < number_of_values; ++k)
					{
						if (!((1 == IO_stream_scan(this->input_file, "%d", &value)) &&
							set_FE_field_int_value(field, k, value)))
						{
							display_message(ERROR_MESSAGE, "EX Reader.  Error reading integer field value.  %s", this->getFileLocation());
							return false;
						}
					}
				} break;
				case STRING_VALUE:
				{
					char *the_string;
					for (int k = 0; k < number_of_values; ++k)
					{
						the_string = this->readString();
						const bool success = (the_string) && (set_FE_field_string_value(field, k, the_string));
						DEALLOCATE(the_string);
						if (!success)
						{
							display_message(ERROR_MESSAGE, "EX Reader.  Error reading string field value.  %s", this->getFileLocation());
							return false;
						}
					}
				} break;
				default:
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Unsupported field value_type %s.  %s",
						Value_type_string(value_type), this->getFileLocation());
					return false;
				} break;
			}
		}
	}
	return true;
}

/**
 * Add derivatives (followed in brackets by number of versions if > 1) e.g.:
 * (value,d/ds1(2),d/ds2,d2/ds1ds2)
 * @return  True on success, false if not read correctly.
 */
bool EXReader::readNodeValueLabelsVersions(FE_node_field_template& nft)
{
	int next_char = this->readNextNonSpaceChar();
	if (next_char != (int)'(')
		return false;
	while (true)
	{
		char *valueLabelName = 0;
		if (!IO_stream_read_string(this->input_file, "[^,()\n\r]", &valueLabelName))
			return false;
		trim_string_in_place(valueLabelName);
		if ((nft.getValueLabelsCount() == 0)
			&& (0 == strlen(valueLabelName))
			&& this->checkConsumeNextChar(')'))
		{
			break; // empty list ()
		}
		cmzn_node_value_label valueLabel = CMZN_NODE_VALUE_LABEL_INVALID;
		if (!STRING_TO_ENUMERATOR(cmzn_node_value_label)(valueLabelName, &valueLabel))
		{
			next_char = this->readNextNonSpaceChar();
			if (next_char == (int)')')
				break; // finished
			display_message(ERROR_MESSAGE, "EX Reader.  Unrecognised value/derivative label name %s", valueLabelName);
			DEALLOCATE(valueLabelName);
			return false;
		}
		DEALLOCATE(valueLabelName);
		int versionCount = 0;
		if (1 == IO_stream_scan(this->input_file, "(%d)", &versionCount))
		{
			if (versionCount < 2)
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Value/derivative version count must be > 1 if specified");
				return false;
			}
		}
		else
		{
			versionCount = 1;
		}
		if (!nft.setValueNumberOfVersions(valueLabel, versionCount))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Invalid derivative, number of versions or repeated derivative");
			return false;
		}
		next_char = this->readNextNonSpaceChar();
		if (next_char == (int)')')
			break; // finished
		else if (next_char != (int)',')
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Invalid character in derivative/value versions list");
			return false;
		}
	}
	return true;
}

/**
 * Read a node field from stream, defining it on the node template and add
 * it to the header fields vector.
 */
bool EXReader::readNodeHeaderField()
{
	if (!(this->nodeset && this->fe_node_template))
		return false;
	// first read non-merged field declaration without node-specific data
	FE_field *field = this->readField();
	if (!field)
		return false;
	bool result = true;
	const int number_of_components = get_FE_field_number_of_components(field);
	const FE_field_type fe_field_type = get_FE_field_FE_field_type(field);
	std::vector<FE_node_field_template> componentNfts(number_of_components);
	/* read the components */
	char *componentName = 0;
	for (int component_number = 0; component_number < number_of_components; ++component_number)
	{
		IO_stream_scan(this->input_file, " ");
		/* read the component name */
		if (componentName)
			DEALLOCATE(componentName);
		if (IO_stream_read_string(this->input_file, "[^.]", &componentName))
		{
			trim_string_in_place(componentName);
			if ((strlen(componentName) == 0)
				|| (!field->setComponentName(component_number, componentName)))
				result = false;
		}
		else
			result = false;
		if (!result)
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Error getting component name for field %s.  %s",
				get_FE_field_name(field), this->getFileLocation());
			break;
		}
		const int nextChar = IO_stream_getc(this->input_file);
		if (nextChar != static_cast<int>('.'))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Missing required '.' after component name.  %s",this->getFileLocation());
			result = false;
			break;
		}
		/* component name is sufficient for non-GENERAL_FE_FIELD */
		if (GENERAL_FE_FIELD == fe_field_type)
		{
			FE_node_field_template &nft = componentNfts[component_number];
			if (this->exVersion < 2)
			{
				// legacy EX format had same number of versions for all derivatives and 'value' derivative was compulsory (and first)
				// Note that all values and derivatives for a given version are consecutive in the legacy format, i.e. derivative cycles fastest
				int number_of_derivatives, number_of_versions, temp_int;
				/* ignore value index */
				if (!((2 == IO_stream_scan(this->input_file, " Value index=%d, #Derivatives=%d ", &temp_int, &number_of_derivatives))
					&& (0 <= number_of_derivatives)))
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Failed to read legacy node field %s component %s #Derivatives.  %s",
						get_FE_field_name(field), componentName, this->getFileLocation());
					result = false;
					break;
				}
				// legacy EX format required value derivative, and it had to be first
				if (!nft.setValueNumberOfVersions(CMZN_NODE_VALUE_LABEL_VALUE, 1))
				{
					result = false;
					break;
				}
				if (0 < number_of_derivatives)
				{
					if (!this->readNodeValueLabelsVersions(nft))
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Legacy derivative types missing or invalid for node field %s component %s.  %s",
							get_FE_field_name(field), componentName, this->getFileLocation());
						result = false;
						break;
					}
					if (nft.getMaximumVersionsCount() != 1)
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Per-derivative versions are only supported from EX Version 2, for node field %s component %s.  %s",
							get_FE_field_name(field), componentName, this->getFileLocation());
						result = false;
						break;
					}
					if (nft.getValueLabelsCount() != (number_of_derivatives + 1))
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Count of derivative types listed did not match number specified for node field %s component %s.  %s",
							get_FE_field_name(field), componentName, this->getFileLocation());
						result = false;
						break;
					}
				}
				// read in the optional number of versions, applied to value and all derivatives
				if (1 == IO_stream_scan(this->input_file, ", #Versions=%d", &number_of_versions))
				{
					if (!nft.setLegacyAllValueVersionsCount(number_of_versions))
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Invalid #Versions for node field %s component %s.  %s",
							get_FE_field_name(field), componentName, this->getFileLocation());
						result = false;
						break;
					}
				}
			}
			else
			{
				// EX Version 2+: supports variable number of versions per derivative.
				// Note that parameters for versions of a given derivative are consecutive in the new format i.e. version cycles within derivative
				int valuesCount = 0;
				if (!((1 == IO_stream_scan(this->input_file, " #Values=%d", &valuesCount))
					&& (0 <= valuesCount)))
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Failed to read node field %s component %s #Values.  %s",
						get_FE_field_name(field), componentName, this->getFileLocation());
					result = false;
					break;
				}
				// follow with derivative names and versions, even if empty ()
				if (!this->readNodeValueLabelsVersions(nft))
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Value/derivative types and versions missing or invalid for field %s component %s.  %s",
						get_FE_field_name(field), componentName, this->getFileLocation());
					result = false;
					break;
				}
				if (nft.getTotalValuesCount() != valuesCount)
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Count of value/derivative versions did not match number specified for field %s component %s.  %s",
						get_FE_field_name(field), componentName, this->getFileLocation());
					result = false;
					break;
				}
			}
			if (!result)
				break;
		}
		if ((this->exVersion >= 2) || (GENERAL_FE_FIELD != fe_field_type))
		{
			// read and warn about any unused key=value data, which must be preceded by ,
			KeyValueMap keyValueMap;
			if (!this->readKeyValueMap(keyValueMap, (GENERAL_FE_FIELD == fe_field_type) ? 0 : (int)','))
			{
				result = false;
				break;
			}
			if (keyValueMap.hasUnusedKeyValues())
			{
				std::string prefix("EX Reader.  Node field ");
				prefix += get_FE_field_name(field);
				prefix += " component ";
				prefix += componentName;
				prefix += ": ";
				keyValueMap.reportUnusedKeyValues(prefix.c_str());
			}
		}
	}
	if (result)
	{
		// merge field into FE_region, which may return an existing matching field
		FE_field *mergedField = FE_region_merge_FE_field(this->fe_region, field);
		if (mergedField)
		{
			// must switch field
			mergedField->access();
			FE_field::deaccess(&field);
			field = mergedField;
			mergedField = 0;
		}
		else
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Could not merge field %s into region.  %s",
				get_FE_field_name(field), this->getFileLocation());
			result = false;
		}
	}
	if (result)
	{
		// define field at the node
		struct FE_time_sequence *fe_time_sequence = 0;
		if (this->timeIndex)
		{
			if (!(fe_time_sequence = FE_region_get_FE_time_sequence_matching_series(
				this->fe_region, 1, &(this->timeIndex->time))))
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Could not get time sequence for field %s at node.  %s",
					get_FE_field_name(field), this->getFileLocation()); 
				result = false;
			}
		}
		if (result)
		{
			if (define_FE_field_at_node(this->fe_node_template->get_template_node(), field,
				componentNfts.data(), fe_time_sequence))
			{
				this->headerFields.push_back(field);
			}
			else
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Could not define field %s at node.  %s",
					get_FE_field_name(field), this->getFileLocation());
				result = false;
			}
		}
	}
	if (componentName)
		DEALLOCATE(componentName);
	FE_field::deaccess(&field);
	return result;
}

/**
 * Creates a node template with the field information read from stream.
 * Fills list of header fields in read order.
 */
bool EXReader::readNodeHeader()
{
	if ((this->mesh) || !(this->nodeset))
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Nodeset not set.  %s", this->getFileLocation());
		return false;
	}
	this->clearHeaderCache();
	this->fe_node_template = this->nodeset->create_FE_node_template();
	if (!this->fe_node_template)
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Failed to create node template.  %s", this->getFileLocation());
		return false;
	}
	int fieldCount = 0;
	if ((1 != IO_stream_scan(this->input_file, "Fields=%d", &fieldCount)) || (fieldCount < 0))
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Error reading number of fields.  %s", this->getFileLocation());
		return false;
	}
	for (int f = 0; f < fieldCount; ++f)
	{
		if (!this->readNodeHeaderField())
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Could not read node field header");
			return false;
		}
	}
	return true;
}

/**
 * Reads a node from stream, adding or merging into current nodeset.
 * If a node of that identifier already exists, parsed data is put into the
 * node template and merged into the existing node.
 * If there is no existing node of that identifier a new node is read and merged
 * directly.
 * @return  On success, ACCESSed node, otherwise 0.
 */
cmzn_node *EXReader::readNode()
{
	if (!this->fe_region)
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Region/Group not set before Node token.  %s", this->getFileLocation());
		return 0;
	}
	if (!((this->nodeset) && (this->fe_node_template)))
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Can't read node as no nodeset set or no node template found.  %s", this->getFileLocation());
		return 0;
	}
	DsLabelIdentifier nodeIdentifier = DS_LABEL_IDENTIFIER_INVALID;
	if (1 != IO_stream_scan(this->input_file, "ode :%d", &nodeIdentifier))
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Error reading Node token or node number.  %s", this->getFileLocation());
		return 0;
	}
	cmzn_node *returnNode = this->nodeset->findNodeByIdentifier(nodeIdentifier);
	const bool existingNode = (returnNode != 0);
	if (returnNode)
	{
		returnNode->access();
	}
	else
	{
		returnNode = this->nodeset->create_FE_node(nodeIdentifier, this->fe_node_template);
		if (!returnNode)
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Could not create node with number %d.  %s", nodeIdentifier, this->getFileLocation());
			return 0;
		}
	}
	bool result = true;
	// fill template node if node with identifier already exists (and merge below), otherwise new node
	FE_node *node = existingNode ? this->fe_node_template->get_template_node() : returnNode;
	const size_t fieldCount = this->headerFields.size();
	for (size_t f = 0; (f < fieldCount) && result; ++f)
	{
		FE_field *field = this->headerFields[f];
		// only GENERAL_FE_FIELD can store values at nodes
		if (GENERAL_FE_FIELD != get_FE_field_FE_field_type(field))
			continue;
		const int componentCount = get_FE_field_number_of_components(field);
		const FE_node_field *node_field = node->getNodeField(field);
		if (!node_field)
		{
			display_message(ERROR_MESSAGE, "EX Reader. Field %s is not defined at node.  %s", get_FE_field_name(field), this->getFileLocation());
			result = false;
			break;
		}
		const int number_of_values = node_field->getTotalValuesCount();
		if (number_of_values <= 0)
		{
			display_message(ERROR_MESSAGE, "EX Reader.  No nodal values for field %s.  %s", get_FE_field_name(field), this->getFileLocation());
			result = false;
			break;
		}
		const Value_type value_type = get_FE_field_value_type(field);
		switch (value_type)
		{
		case ELEMENT_XI_VALUE:
		{
			cmzn_element *element;
			FE_value xi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
			if (number_of_values == componentCount)
			{
				for (int k = 0; k < number_of_values; ++k)
				{
					if (!(this->readElementXiValue(field, element, xi)
						&& set_FE_nodal_element_xi_value(node, field, /*component_number*/k, element, xi)))
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Error reading element_xi value for field %s at node %d.  %s",
							get_FE_field_name(field), nodeIdentifier, this->getFileLocation());
						result = false;
						break;
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Derivatives/versions not supported for element_xi valued field %s at node %d.  %s",
					get_FE_field_name(field), nodeIdentifier, this->getFileLocation());
				result = false;
			}
		} break;
		case FE_VALUE_VALUE:
		{
			const int maximumValuesCount = node_field->getMaximumComponentTotalValuesCount();
			std::vector<FE_value> valuesVector(maximumValuesCount);
			FE_value *values = valuesVector.data();
			for (int c = 0; c < componentCount; ++c)
			{
				const FE_node_field_template &nft = *(node_field->getComponent(c));
				const int valuesCount = nft.getTotalValuesCount();
				for (int k = 0; k < valuesCount; ++k)
				{
					if (1 != IO_stream_scan(this->input_file, FE_VALUE_INPUT_STRING, &(values[k])))
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Error reading real value for field %s at node %d.  %s",
							get_FE_field_name(field), nodeIdentifier, this->getFileLocation());
						result = false;
						break;
					}
					if (!finite(values[k]))
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Infinity or NAN read for field %s at node %d.  %s",
							get_FE_field_name(field), nodeIdentifier, this->getFileLocation());
						result = false;
						break;
					}
				}
				if (!result)
				{
					break;
				}
				if (this->exVersion < 2)
				{
					// all derivatives / value labels had same number of versions in old format
					const int versionsCount = nft.getVersionsCountAtIndex(0);
					if (versionsCount > 1)
					{
						// before EX version 2, derivatives were nested within versions; reorder to nest versions within derivatives
						std::vector<FE_value> tmpValuesVector(valuesVector);
						FE_value *tmpValues = tmpValuesVector.data();
						const int valueLabelsCount = nft.getValueLabelsCount();
						for (int d = 0; d < valueLabelsCount; ++d)
						{
							for (int v = 0; v < versionsCount; ++v)
							{
								values[d*versionsCount + v] = tmpValues[d + v*valueLabelsCount];
							}
						}
					}
				}
				if (!cmzn_node_set_field_component_FE_value_values(node, field, c,
					(this->timeIndex) ? this->timeIndex->time : 0.0, valuesCount, values))
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Failed to set %d real values for field %s node %d.  %s",
						valuesCount, get_FE_field_name(field), nodeIdentifier, this->getFileLocation());
					result = false;
				}
			}
		} break;
		case INT_VALUE:
		{
			const int maximumValuesCount = node_field->getMaximumComponentTotalValuesCount();
			std::vector<int> valuesVector(maximumValuesCount);
			int *values = valuesVector.data();
			for (int c = 0; c < componentCount; ++c)
			{
				const FE_node_field_template &nft = *(node_field->getComponent(c));
				const int valuesCount = nft.getTotalValuesCount();
				for (int k = 0; k < valuesCount; ++k)
				{
					if (1 != IO_stream_scan(this->input_file, "%d", &(values[k])))
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Error reading int value for field %s at node %d.  %s",
							get_FE_field_name(field), nodeIdentifier, this->getFileLocation());
						result = false;
						break;
					}
				}
				if (!result)
				{
					break;
				}
				if (this->exVersion < 2)
				{
					// all derivatives / value labels had same number of versions in old format
					const int versionsCount = nft.getVersionsCountAtIndex(0);
					if (versionsCount > 1)
					{
						// before EX version 2, derivatives were nested within versions; reorder to nest versions within derivatives
						std::vector<int> tmpValuesVector(valuesVector);
						int *tmpValues = tmpValuesVector.data();
						const int valueLabelsCount = nft.getValueLabelsCount();
						for (int d = 0; d < valueLabelsCount; ++d)
						{
							for (int v = 0; v < versionsCount; ++v)
							{
								values[d*versionsCount + v] = tmpValues[d + v*valueLabelsCount];
							}
						}
					}
				}
				if (!cmzn_node_set_field_component_int_values(node, field, c,
					(this->timeIndex) ? this->timeIndex->time : 0.0, valuesCount, values))
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Failed to set %d int values for field %s node %d.  %s",
						valuesCount, get_FE_field_name(field), nodeIdentifier, this->getFileLocation());
					result = false;
				}
			}
		} break;
		case STRING_VALUE:
		{
			if (number_of_values == componentCount)
			{
				for (int k = 0; k < number_of_values; ++k)
				{
					char *theString = this->readString();
					if (theString)
					{
						if (!set_FE_nodal_string_value(node, field, /*component_number*/k, theString))
						{
							display_message(ERROR_MESSAGE, "EX Reader.  Error setting string value for field %s at node %d.  %s",
								get_FE_field_name(field), nodeIdentifier, this->getFileLocation());
							result = false;
							break;
						}
						DEALLOCATE(theString);
					}
					else
					{
						display_message(ERROR_MESSAGE, "Error reading string value for field %s at node %d.  %s",
							get_FE_field_name(field), nodeIdentifier, this->getFileLocation());
						result = false;
						break;
					}
				}
			}
			else
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Derivatives/versions not supported for string valued field %s at node %d.  %s",
					get_FE_field_name(field), nodeIdentifier, this->getFileLocation());
				result = false;
			}
		} break;
		default:
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Unsupported value_type %s for node %d.  %s",
				Value_type_string(value_type), nodeIdentifier, this->getFileLocation());
			result = false;
		} break;
		}
	}
	if (result && existingNode && (fieldCount > 0)) // nothing to merge if no fields
	{
		if (CMZN_OK != this->nodeset->merge_FE_node_template(returnNode, this->fe_node_template))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Failed to merge into existing node %d.  %s",
				nodeIdentifier, this->getFileLocation());
			result = false;
		}
	}
	if (!result)
		cmzn_node::deaccess(returnNode);
	return returnNode;
}

/** Create blank element template (with no fields) for current mesh and shape.
* Element fields can be added to this.
* @return  True on success, false if failed. */
bool EXReader::createElementtemplate()
{
	if (!(this->mesh && this->elementShape))
	{
		display_message(ERROR_MESSAGE, "EXReader::createElementtemplate.  No mesh and/or current shape");
		return false;
	}
	// create blank element template of shape, with no fields
	this->elementtemplate = cmzn_elementtemplate::create(this->mesh);
	if (!this->elementtemplate)
	{
		display_message(ERROR_MESSAGE, "EXReader::createElementtemplate.  Error creating blank element template");
		return false;
	}
	if (CMZN_OK != this->elementtemplate->setElementShape(this->elementShape))
	{
		display_message(ERROR_MESSAGE, "EXReader::createElementtemplate.  Error setting element template shape");
		return false;
	}
	return true;
}

/**
 * Create element shape from description in stream.
 * Assumes the starting S in Shape token has already been read.
 * From EX version 2 the nodeset or mesh must have been set first and match in
 * dimension. In earlier versions the mesh is assumed from the dimension.
 * GRC: check the above
 * If dimension is positive, on successful return, the EXReader contains an accessed
 * element shape. If dimension is 0 the element shape is NULL.
 * @return  True on success, False on failure.
 */
bool EXReader::readElementShape()
{
	if (!this->fe_region)
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Region/Group not set before Shape token.  %s", this->getFileLocation());
		return false;
	}
	this->clearHeaderCache();
	int dimension = -1;
	if ((1 != IO_stream_scan(this->input_file, "hape. Dimension=%d", &dimension))
		|| (dimension < 0) || (dimension > MAXIMUM_ELEMENT_XI_DIMENSIONS))
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Error reading element shape dimension.  %s", this->getFileLocation());
		return false;
	}
	if (this->exVersion < 2)
	{
		if (dimension == 0)
		{
			// always set nodeset to reset node template
			this->setNodeset((this->nodeset) ? this->nodeset :
				FE_region_find_FE_nodeset_by_field_domain_type(this->fe_region,
					this->useData ? CMZN_FIELD_DOMAIN_TYPE_DATAPOINTS : CMZN_FIELD_DOMAIN_TYPE_NODES));
		}
		else
		{
			this->setMesh(FE_region_find_FE_mesh_by_dimension(this->fe_region, dimension));
		}
	}
	else
	{
		if (dimension == 0)
		{
			if ((!this->nodeset) || (this->mesh))
			{
				display_message(ERROR_MESSAGE, "EX Reader.  0-D shape must follow !#nodeset declaration in EX version 2+.  %s", this->getFileLocation());
				return false;
			}
		}
		else
		{
			if (!this->mesh)
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Shape specified before !#mesh in EX version 2+.  %s", this->getFileLocation());
				return false;
			}
			if (this->mesh->getDimension() != dimension)
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Shape dimension does not match current !#mesh.  %s", this->getFileLocation());
				return false;
			}
		}
	}
	if (0 == dimension)
		return true;
	char *end_description, *shape_description_string, *start_description;
	int component, *first_simplex, i, j, number_of_polygon_vertices,
		previous_component, *temp_entry, *type, *type_entry,
		xi_number;
	if (!ALLOCATE(type, int, (dimension*(dimension + 1))/2))
	{
		display_message(ERROR_MESSAGE, "EXReader::readElementShape.  Could not allocate shape type");
		return false;
	}
	IO_stream_scan(this->input_file,",");
	if ((!IO_stream_read_string(this->input_file, "[^\n\r]", &shape_description_string))
		|| (!shape_description_string))
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Error reading shape description.  %s", this->getFileLocation());
		DEALLOCATE(type);
		return false;
	}
	/* trim the shape description string */
	end_description=shape_description_string+
		(strlen(shape_description_string)-1);
	while ((end_description>shape_description_string)&&
		(' '== *end_description))
	{
		end_description--;
	}
	end_description[1]='\0';
	start_description=shape_description_string;
	while (' '== *start_description)
	{
		start_description++;
	}
	if ('\0'== *start_description)
		DEALLOCATE(shape_description_string);
	if (shape_description_string)
	{
		/* decipher the shape description */
		xi_number=0;
		type_entry=type;
		while (type&&(xi_number<dimension))
		{
			xi_number++;
			if (xi_number<dimension)
			{
				if (NULL != (end_description=strchr(start_description,'*')))
				{
					*end_description='\0';
				}
				else
				{
					DEALLOCATE(type);
				}
			}
			if (type)
			{
				if (0==strncmp(start_description,"line",4))
				{
					start_description += 4;
					*type_entry=LINE_SHAPE;
					while (' '== *start_description)
					{
						start_description++;
					}
					if ('\0'== *start_description)
					{
						type_entry++;
						for (i=dimension-xi_number;i>0;i--)
						{
							*type_entry=0;
							type_entry++;
						}
					}
					else
					{
						DEALLOCATE(type);
					}
				}
				else
				{
					if (0==strncmp(start_description,"polygon",7))
					{
						start_description += 7;
						while (' '== *start_description)
						{
							start_description++;
						}
						if ('\0'== *start_description)
						{
							/* check for link to first polygon coordinate */
							temp_entry=type_entry;
							i=xi_number-1;
							j=dimension-xi_number;
							number_of_polygon_vertices=0;
							while (type&&(i>0))
							{
								j++;
								temp_entry -= j;
								if (*temp_entry)
								{
									if (0<number_of_polygon_vertices)
									{
										DEALLOCATE(type);
									}
									else
									{
										if (!((POLYGON_SHAPE==temp_entry[i-xi_number])&&
											((number_of_polygon_vertices= *temp_entry)>=3)))
										{
											DEALLOCATE(type);
										}
									}
								}
								i--;
							}
							if (type&&(3<=number_of_polygon_vertices))
							{
								*type_entry=POLYGON_SHAPE;
								type_entry++;
								for (i=dimension-xi_number;i>0;i--)
								{
									*type_entry=0;
									type_entry++;
								}
							}
							else
							{
								DEALLOCATE(type);
							}
						}
						else
						{
							/* assign link to second polygon coordinate */
							if ((2==sscanf(start_description,"(%d ;%d )%n",
								&number_of_polygon_vertices,&component,&i))&&
								(3<=number_of_polygon_vertices)&&
								(xi_number<component)&&(component<=dimension)&&
								('\0'==start_description[i]))
							{
								*type_entry=POLYGON_SHAPE;
								type_entry++;
								i=xi_number+1;
								while (i<component)
								{
									*type_entry=0;
									type_entry++;
									i++;
								}
								*type_entry=number_of_polygon_vertices;
								type_entry++;
								while (i<dimension)
								{
									*type_entry=0;
									type_entry++;
									i++;
								}
							}
							else
							{
								DEALLOCATE(type);
							}
						}
					}
					else
					{
						if (0==strncmp(start_description,"simplex",7))
						{
							start_description += 7;
							while (' '== *start_description)
							{
								start_description++;
							}
							if ('\0'== *start_description)
							{
								/* check for link to previous simplex coordinate */
								temp_entry=type_entry;
								i=xi_number-1;
								j=dimension-xi_number;
								first_simplex=(int *)NULL;
								while (type&&(i>0))
								{
									j++;
									temp_entry -= j;
									if (*temp_entry)
									{
										if (SIMPLEX_SHAPE==temp_entry[i-xi_number])
										{
											first_simplex=temp_entry;
										}
										else
										{
											DEALLOCATE(type);
										}
									}
									i--;
								}
								if (type&&first_simplex)
								{
									*type_entry=SIMPLEX_SHAPE;
									type_entry++;
									first_simplex++;
									for (i=dimension-xi_number;i>0;i--)
									{
										*type_entry= *first_simplex;
										type_entry++;
										first_simplex++;
									}
								}
								else
								{
									DEALLOCATE(type);
								}
							}
							else
							{
								/* assign link to succeeding simplex coordinate */
								previous_component=xi_number+1;
								if ((1 == sscanf(start_description, "(%d %n",
									&component, &i)) &&
									(previous_component <= component) &&
									(component <= dimension))
								{
									*type_entry=SIMPLEX_SHAPE;
									type_entry++;
									do
									{
										start_description += i;
										while (previous_component<component)
										{
											*type_entry=0;
											type_entry++;
											previous_component++;
										}
										*type_entry=1;
										type_entry++;
										previous_component++;
									} while ((')'!=start_description[0])&&
										(1==sscanf(start_description,"%*[; ]%d %n",
										&component,&i))&&(previous_component<=component)&&
										(component<=dimension));
									if (')'==start_description[0])
									{
										/* fill rest of shape_type row with zeroes */
										while (previous_component <= dimension)
										{
											*type_entry=0;
											type_entry++;
											previous_component++;
										}
									}
									else
									{
										DEALLOCATE(type);
									}
								}
								else
								{
									DEALLOCATE(type);
								}
							}
						}
						else
						{
							DEALLOCATE(type);
						}
					}
				}
				if (type&&(xi_number<dimension))
				{
					start_description=end_description+1;
				}
			}
		}
		DEALLOCATE(shape_description_string);
	}
	else
	{
		/* retrieve a line/square/cube element of the specified dimension */
		type_entry=type;
		for (i=dimension-1;i>=0;i--)
		{
			*type_entry=LINE_SHAPE;
			type_entry++;
			for (j=i;j>0;j--)
			{
				*type_entry=0;
				type_entry++;
			}
		}
	}
	if (!type)
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Invalid shape description.  %s", this->getFileLocation());
		return false;
	}
	this->elementShape = CREATE(FE_element_shape)(dimension, type, this->fe_region); // not accessed!
	DEALLOCATE(type);
	if (!this->elementShape)
	{
		display_message(ERROR_MESSAGE, "EXReader::readElementShape.  Error creating shape");
		return false;
	}
	ACCESS(FE_element_shape)(this->elementShape);
	// create and validate a blank, no-field element template
	if (!this->createElementtemplate())
		return false;
	if (!this->elementtemplate->validate())
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Default element field header failed validation when created for new shape.  %s",
			this->getFileLocation());
		return false;
	}
	return true;
}

/*
 * Create basis from basis description read from input stream.
 * Some examples of basis descriptions are:
 * c.Hermite*c.Hermite*l.Lagrange  This has cubic variation in xi1 and xi2 and
 * linear variation in xi3.
 * c.Hermite*l.simplex(3)*l.simplex  This has cubic variation in xi1 and 2-D
 * linear simplex variation for xi2 and xi3.
 * polygon(5,3)*l.Lagrange*polygon  This has linear variation in xi2 and a 2-D
 * 5-gon for xi1 and xi3.
 * @return  NON-Accessed basis object or 0 if failed.
 */
struct FE_basis *EXReader::readBasis()
{
	char *basis_description_string;
	if (!IO_stream_read_string(this->input_file, "[^,]", &basis_description_string))
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Error reading basis description.  %s", this->getFileLocation());
		return 0;
	}
	char *start_basis_name = basis_description_string;
	// remove leading and trailing blanks
	while (' '== *start_basis_name)
		++start_basis_name;
	char *end_basis_name = start_basis_name + (strlen(start_basis_name)-1);
	while (' '== *end_basis_name)
		--end_basis_name;
	end_basis_name[1] = '\0';
	int *basis_type = FE_basis_string_to_type_array(start_basis_name);
	FE_basis *basis = 0;
	if (basis_type)
	{
		basis = FE_region_get_FE_basis_matching_basis_type(this->fe_region, basis_type);
		DEALLOCATE(basis_type);
	}
	else
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Error parsing basis description.  %s", this->getFileLocation());
	}
	DEALLOCATE(basis_description_string);
	return basis;
}

/**
 * Reads an element field from stream, adding it to the field information
 * described in the element template. Adds field to field order info.
 * @return  True on success, false if failed.
 */
bool EXReader::readElementHeaderField()
{
	if (!(this->mesh && this->nodeset && this->elementtemplate))
		return false;
	// first read non-merged field declaration without element-specific data
	FE_field *field = this->readField();
	if (!field)
		return false;
	// check field of this name isn't already in header
	const size_t fieldCount = this->headerFields.size();
	for (size_t f = 0; f < fieldCount; ++f)
	{
		FE_field *tmpField = this->headerFields[f];
		if (0 == strcmp(get_FE_field_name(tmpField), get_FE_field_name(field)))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Field %s appears more than once in header.  %s",
				field->getName(), this->getFileLocation());
			FE_field::deaccess(&field);
			return false;
		}
	}
	const int componentCount = get_FE_field_number_of_components(field);
	const enum FE_field_type fe_field_type = get_FE_field_FE_field_type(field);
	bool result = true;
	int resultCode;
	char *componentName = 0;

	std::vector<cmzn_elementfieldtemplate *> componentEFTs(componentCount, static_cast<cmzn_elementfieldtemplate *>(0));
	std::vector< std::vector<int> > componentPackedNodeIndexes(componentCount);
	
	for (int c = 0; c < componentCount; ++c)
	{
		IO_stream_scan(this->input_file, " ");
		// read the component name
		if (componentName)
			DEALLOCATE(componentName);
		if (IO_stream_read_string(this->input_file, "[^.]", &componentName))
		{
			trim_string_in_place(componentName);
			if ((strlen(componentName) == 0)
				|| (!field->setComponentName(c, componentName)))
				result = false;
		}
		else
			result = false;
		if (!result)
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Error reading component name for field %s.  %s",
				get_FE_field_name(field), this->getFileLocation());
			break;
		}
		IO_stream_scan(this->input_file, ". ");
		FE_basis *basis = 0;
		const int dimension = this->mesh->getDimension();
		if (GENERAL_FE_FIELD == fe_field_type)
			basis = this->readBasis();
		else
			basis = FE_region_get_constant_FE_basis_of_dimension(this->fe_region, dimension);
		cmzn_elementfieldtemplate *eft = cmzn_elementfieldtemplate::create(this->mesh, basis);
		if (!eft)
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Failed to create element field template due to invalid basis for %d-D mesh.  %s",
				this->mesh->getDimension(), this->getFileLocation());
			result = false;
			break;
		}
		componentEFTs[c] = eft;
		if (GENERAL_FE_FIELD != fe_field_type)
		{
			/* component name is sufficient for non-GENERAL_FE_FIELD */
			eft->setParameterMappingMode(CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_FIELD);
		}
		else
		{
			IO_stream_scan(this->input_file, ", ");
			// read the basis modify theta mode name
			FE_basis_modify_theta_mode thetaModifyMode = FE_BASIS_MODIFY_THETA_MODE_INVALID;
			char *modify_function_name = 0;
			if (!IO_stream_read_string(this->input_file, "[^,]", &modify_function_name))
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Error reading modify function.  %s", this->getFileLocation());
				result = false;
				break;
			}
			if (0 == strcmp("no modify", modify_function_name))
			{
				thetaModifyMode = FE_BASIS_MODIFY_THETA_MODE_INVALID;
			}
			else if (0 == strcmp("increasing in xi1", modify_function_name))
			{
				thetaModifyMode = FE_BASIS_MODIFY_THETA_MODE_INCREASING_IN_XI1;
			}
			else if (0 == strcmp("decreasing in xi1", modify_function_name))
			{
				thetaModifyMode = FE_BASIS_MODIFY_THETA_MODE_DECREASING_IN_XI1;
			}
			else if (0 == strcmp("non-increasing in xi1", modify_function_name))
			{
				thetaModifyMode = FE_BASIS_MODIFY_THETA_MODE_NON_INCREASING_IN_XI1;
			}
			else if (0 == strcmp("non-decreasing in xi1", modify_function_name))
			{
				thetaModifyMode = FE_BASIS_MODIFY_THETA_MODE_NON_DECREASING_IN_XI1;
			}
			else if (0 == strcmp("closest in xi1", modify_function_name))
			{
				thetaModifyMode = FE_BASIS_MODIFY_THETA_MODE_CLOSEST_IN_XI1;
			}
			else
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Invalid modify function %s.  %s", modify_function_name, this->getFileLocation());
				result = false;
			}
			DEALLOCATE(modify_function_name);
			if (!result)
				break;

			IO_stream_scan(this->input_file, ", ");
			// read the global to element map type
			cmzn_elementfieldtemplate_parameter_mapping_mode elementParameterMappingMode = CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_INVALID;
			bool elementGridBased = false;
			char *global_to_element_map_string = 0;
			if (!IO_stream_read_string(this->input_file, "[^.]", &global_to_element_map_string))
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Error reading global to element map type.  %s", this->getFileLocation());
				result = false;
				break;
			}
			IO_stream_scan(this->input_file, ".");
			if (0 == strcmp("standard node based", global_to_element_map_string))
			{
				elementParameterMappingMode = CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE;
			}
			else if (0 == strcmp("element based", global_to_element_map_string))
			{
				elementParameterMappingMode = CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_ELEMENT;
			}
			else if (0 == strcmp("grid based", global_to_element_map_string))
			{
				elementParameterMappingMode = CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_ELEMENT;
				elementGridBased = true;
			}
			else if (0 == strcmp("field based", global_to_element_map_string))
			{
				elementParameterMappingMode = CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_FIELD;
			}
			else
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Invalid element parameter mapping mode %s.  %s",
					global_to_element_map_string, this->getFileLocation());
				result = false;
			}
			DEALLOCATE(global_to_element_map_string);
			if (!result)
				break;
			if (CMZN_OK != eft->setParameterMappingMode(elementParameterMappingMode))
			{
				display_message(WARNING_MESSAGE, "EX Reader.  Can't use element parameter mapping mode with chosen basis.  %s",
					this->getFileLocation());
				result = false;
				break;
			}
			if (CMZN_OK != eft->setLegacyModifyThetaMode(thetaModifyMode))
			{
				display_message(WARNING_MESSAGE, "EX Reader.  Can't use theta modify function with chosen element parameter mapping mode.  %s",
					this->getFileLocation());
			}
			// optionally read additional key=value data
			KeyValueMap keyValueMapBase;
			const char *scaleFactorSetName = 0; // prior to EX version 2, used the basis description as the scale factor name
			if (!this->readKeyValueMap(keyValueMapBase))
			{
				result = false;
				break;
			}
			if ((this->exVersion >= 2) && (CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE == elementParameterMappingMode))
			{
				scaleFactorSetName = keyValueMapBase.getKeyValue("scale factor set");
			}
			if (keyValueMapBase.hasUnusedKeyValues())
			{
				std::string prefix("EX Reader.  Element field ");
				prefix += get_FE_field_name(field);
				prefix += " component ";
				prefix += componentName;
				prefix += ": ";
				keyValueMapBase.reportUnusedKeyValues(prefix.c_str());
			}
			switch (elementParameterMappingMode)
			{
			case CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_NODE:
			{
				// node to element map: includes standard and general
				int nodeCount = 0;
				if (1 != IO_stream_scan(this->input_file, " #Nodes=%d", &nodeCount))
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Error reading field %s component %s number of nodes.  %s",
						get_FE_field_name(field), componentName, this->getFileLocation());
					result = false;
					break;
				}
				// In EX Version 2+ this is the number of nodes in the EFT; prior to that it was the
				// number of node mapping lines corresponding to the preferred number of nodes for
				// the basis, which is reduced below.
				// Local nodes are initially indexes in order of first reference, so
				// 2 6 3 3 1 means 4 unique local node indexes 1 2 3 4 for nodes 2 6 3 1
				// Later we reorder to numerical order in element, i.e. 1 2 3 6
				// This requires remapping the indexes used to 2 4 3 3 1
				if (CMZN_OK != eft->setNumberOfLocalNodes(nodeCount))
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Failed to set element field template number of nodes.  %s", this->getFileLocation());
					result = false;
					break;
				}
				if (this->exVersion >= 2)
				{
					// if a comma is next read additional key=value data
					KeyValueMap keyValueMap;
					if (!this->readKeyValueMap(keyValueMap, (int)','))
					{
						result = false;
						break;
					}
					if (keyValueMap.hasUnusedKeyValues())
					{
						std::string prefix("EX Reader.  Element field");
						prefix += get_FE_field_name(field);
						prefix += " component ";
						prefix += componentName;
						prefix += ": ";
						keyValueMap.reportUnusedKeyValues(prefix.c_str());
					}
				}

				int scaleFactorCount = 0;
				int scaleFactorOffset = 0;
				ScaleFactorSet *sfSet = 0;
				if (scaleFactorSetName) // EX Version 2+ only; none if not scaling
				{
					sfSet = this->findScaleFactorSet(scaleFactorSetName);
					if (!sfSet)
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Could not find scale factor set %s.  %s", scaleFactorSetName, this->getFileLocation());
						result = false;
						break;
					}
				}
				else if (this->exVersion < 2)
				{
					// basis name was used as identifier for scale factor set prior to EX Version 2
					char *basisDescription = FE_basis_get_description_string(basis);
					if (!basisDescription)
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Failed to get basis description.  %s", this->getFileLocation());
						result = false;
						break;
					}
					sfSet = this->findScaleFactorSet(basisDescription);
					DEALLOCATE(basisDescription);
				}
				if (sfSet)
				{
					// do following later to allow legacy format to skip scale factor set if not used
					// sfSet->addFieldComponent(get_FE_field_name(field), c);
					scaleFactorCount = sfSet->scaleFactorCount;
					scaleFactorOffset = sfSet->scaleFactorOffset;
				}
				if (CMZN_OK != eft->setNumberOfLocalScaleFactors(scaleFactorCount))
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Failed to set number of scale factors.  %s", this->getFileLocation());
					result = false;
					break;
				}
				if (sfSet)
				{
					for (int s = 0; s < scaleFactorCount; ++s)
					{
						if ((CMZN_OK != eft->setScaleFactorType(s + 1, sfSet->getScaleFactorType(s)))
							|| (CMZN_OK != eft->setScaleFactorIdentifier(s + 1, sfSet->getScaleFactorIdentifier(s))))
						{
							display_message(ERROR_MESSAGE, "EX Reader.  Failed to set scale factor type or identifier.  %s", this->getFileLocation());
							result = false;
							break;
						}
					}
				}
				// set following to true if scale factors are actually used, EX version < 2 only
				bool usingScaleFactors = false;
				// following stores element node indexes in order referenced by EFT (until sorted later)
				std::vector<int> packedNodeIndexes;
				int fn = 1;
				const int functionCount = eft->getNumberOfFunctions();
				while (fn <= functionCount)
				{
					// read one or more local node indexes summing terms e.g. "0." (special = zero terms), "1.", "1+2.", "3+1+3."
					char *nodeIndexString = 0;
					int termCount = 0;
					// following stores node indexes for termCount terms as indexes in packedNodeIndexes + 1
					std::vector<int> termNodeIndexes;
					while (true)
					{
						IO_stream_scan(this->input_file, " ");
						if (!IO_stream_read_string(this->input_file, "[^.+]", &nodeIndexString))
						{
							display_message(ERROR_MESSAGE, "EX Reader.  Failed to read local node index expression.  %s", this->getFileLocation());
							result = false;
							break;
						}
						const int next_char = this->readNextNonSpaceChar();
						const int nodeIndex = atoi(nodeIndexString);
						if ((!isIntegerString(nodeIndexString))
							|| (nodeIndex < 0)
							|| (nodeIndex > this->elementtemplate->getNumberOfNodes())
							|| ((nodeIndex == 0) && (termCount > 0)))
						{
							display_message(ERROR_MESSAGE, "EX Reader.  Node index %s invalid or out of range.  %s", nodeIndexString, this->getFileLocation());
							result = false;
							break;
						}
						if (0 == nodeIndex)
						{
							if (next_char != (int)'.')
							{
								display_message(ERROR_MESSAGE, "EX Reader.  Node index 0 (zero terms) must be on its own ('0.').  %s", this->getFileLocation());
								result = false;
							}
							break;
						}
						++termCount;
						const int packedCount = static_cast<int>(packedNodeIndexes.size());
						int packedIndex = 0;
						for (; packedIndex < packedCount; ++packedIndex)
						{
							if (packedNodeIndexes[packedIndex] == nodeIndex)
								break;
						}
						if (packedIndex == packedCount)
						{
							if (packedCount == nodeCount)
							{
								display_message(ERROR_MESSAGE, "EX Reader.  Too many nodes referenced, expected %d.  %s",
									nodeCount, this->getFileLocation());
								result = false;
								break;
							}
							packedNodeIndexes.push_back(nodeIndex);
						}
						termNodeIndexes.push_back(packedIndex + 1);
						DEALLOCATE(nodeIndexString);
						if (next_char == (int)'.')
							break;
					}
					if (nodeIndexString)
						DEALLOCATE(nodeIndexString);
					if (!result)
						break;
					if ((this->exVersion < 2) && (termCount != 1))
					{
						display_message(ERROR_MESSAGE, "EX Reader.  EX Version < 2 supports exactly 1 term per function; this has %d.  %s",
							termCount, this->getFileLocation());
						result = false;
						break;
					}
					int valueCount = 0;
					if ((1 != IO_stream_scan(this->input_file, " #Values=%d", &valueCount)) || (valueCount < 1))
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Invalid #Values.  %s", this->getFileLocation());
						result = false;
						break;
					}
					if ((fn + valueCount - 1) > functionCount)
					{
						display_message(ERROR_MESSAGE, "EX Reader.  #Values would exceed number of basis functions.  %s", this->getFileLocation());
						result = false;
						break;
					}

					char *dofMappingTypeString = 0;
					// old EX files use indices into nodal values
					// new EX files use value labels e.g. value d/ds1(2) zero, now compulsory in EX version 2+
					if (!IO_stream_read_string(this->input_file, "[^:]", &dofMappingTypeString))
					{
						result = false;
						break;
					}
					char test_string[5];
					const bool readValueIndices = (1 == sscanf(dofMappingTypeString, " Value indice%1[s] ", test_string));
					if ((!readValueIndices) && (1 != sscanf(dofMappingTypeString, " Value label%1[s] ", test_string)))
						result = false;
					DEALLOCATE(dofMappingTypeString);
					if (!result)
					{
						display_message(ERROR_MESSAGE, "Missing \"Value indices:\" or \"Value labels:\" token.  %s", this->getFileLocation());
						break;
					}
					IO_stream_scan(this->input_file, ": ");
					const int termLimit = (termCount) > 0 ? termCount : 1;
					if (readValueIndices)
					{
						if (this->exVersion >= 2)
						{
							display_message(ERROR_MESSAGE, "EX Reader.  Must use Value labels, not Value indices in EX version 2+.  %s", this->getFileLocation());
							result = false;
							break;
						}
						for (int v = 0; v < valueCount; ++v)
						{
							int nodeValueIndex = 0;
							if (1 != IO_stream_scan(this->input_file, "%d", &nodeValueIndex))
							{
								display_message(ERROR_MESSAGE, "EX Reader.  Error reading nodal value index.  %s", this->getFileLocation());
								result = false;
								break;
							}
							if (nodeValueIndex == 0)
								resultCode = eft->setFunctionNumberOfTerms(fn + v, 0);
							else
								resultCode = eft->setTermNodeParameterLegacyIndex(fn + v, /*term*/1, termNodeIndexes[0], nodeValueIndex);
							if (resultCode != CMZN_OK)
							{
								display_message(ERROR_MESSAGE, "EX Reader.  Failed to set legacy node DOF index.  %s", this->getFileLocation());
								result = false;
								break;
							}
						}
						if (!result)
							break;
					}
					else
					{
						char *rest_of_line = 0;
						// read value labels value type (versions) e.g. value d/ds1(2) d2/ds1ds2 zero d/ds1(2)+d/ds2(3) etc.
						if (!IO_stream_read_string(this->input_file, "[^\n\r]", &rest_of_line))
						{
							display_message(ERROR_MESSAGE, "EX Reader.  Missing node value label expressions.  %s", this->getFileLocation());
							result = false;
							break;
						}
						char *s = rest_of_line;
						char nextchar;
						const char *token;
						for (int v = 0; v < valueCount; ++v)
						{
							resultCode = eft->setFunctionNumberOfTerms(fn + v, termCount);
							if (resultCode != CMZN_OK)
							{
								display_message(ERROR_MESSAGE, "EX Reader.  Failed to set function %d number of terms to %d.  %s",
									fn + v, termCount, this->getFileLocation());
								result = false;
								break;
							}
							for (int t = 1; t <= termLimit; ++t)
							{
								token = nexttoken(s, "(+", nextchar);
								if ((termCount <= 1) && (0 == strcmp(token, "zero")))
								{
									if (!(isspace(nextchar) || (nextchar == '\0')))
									{
										display_message(ERROR_MESSAGE, "EX Reader.  Invalid character '%c' after value label 'zero'.  %s", nextchar, this->getFileLocation());
										result = false;
										break;
									}
									if (termCount == 1)
									{
										// prior to EX version 2, could have 'zero' value with a single node
										resultCode = eft->setFunctionNumberOfTerms(fn + v, 0);
									}
								}
								else if (termCount == 0)
								{
									display_message(ERROR_MESSAGE, "EX Reader.  Require value label 'zero' when there are no terms.  %s", this->getFileLocation());
									result = false;
									break;
								}
								else
								{
									cmzn_node_value_label valueLabel = CMZN_NODE_VALUE_LABEL_INVALID;
									if (!STRING_TO_ENUMERATOR(cmzn_node_value_label)(token, &valueLabel))
									{
										display_message(ERROR_MESSAGE, "EX Reader.  Invalid node value/derivative label '%s'.  %s", token, this->getFileLocation());
										result = false;
										break;
									}
									int version = 1;
									if (nextchar == '(')
									{
										const char *versionToken = nexttoken(s, ")", nextchar);
										version = atoi(versionToken);
										if ((!isIntegerString(versionToken)) || (nextchar != ')') || (version < 1))
										{
											display_message(ERROR_MESSAGE, "EX Reader.  Invalid version number specification.  %s", this->getFileLocation());
											result = false;
											break;
										}
										// look for possible + after version
										while (*s == ' ')
											++s;
										nextchar = *s;
										if (nextchar == '+')
											++s;
									}
									resultCode = eft->setTermNodeParameter(fn + v, t, termNodeIndexes[t - 1], valueLabel, version);
									if (resultCode != CMZN_OK)
									{
										display_message(ERROR_MESSAGE, "EX Reader.  Failed to set function %d node parameter term %d.  %s",
											fn + v, t, this->getFileLocation());
										result = false;
										break;
									}
								}
								if (t < termCount)
								{
									if (nextchar != '+')
									{
										display_message(ERROR_MESSAGE, "EX Reader.  Require '+' followed by additional term(s).  %s", this->getFileLocation());
										result = false;
										break;
									}
								}
								else if (nextchar == '+')
								{
									display_message(ERROR_MESSAGE, "EX Reader.  Too many terms, unexpected character '%c'.  %s", nextchar, this->getFileLocation());
									result = false;
									break;
								}
							}
							if (!result)
								break;
						}
						if (result)
						{
							token = nexttoken(s, "", nextchar);
							if (0 != strlen(token))
							{
								display_message(ERROR_MESSAGE, "EX Reader.  Unexpected text '%s' after labels.  %s", token, this->getFileLocation());
								result = false;
							}
						}
						DEALLOCATE(rest_of_line);
						if (!result)
							break;
					}
					// read the scale factor indices, but only if using scaling in EX Version 2+
					if ((scaleFactorCount > 0) || (this->exVersion < 2))
					{
						if (1 != IO_stream_scan(this->input_file, " Scale factor indices%1[:] ", test_string))
						{
							display_message(ERROR_MESSAGE, "EX Reader.  Missing \"Scale factor indices:\" token.  %s", this->getFileLocation());
							result = false;
							break;
						}
						char *rest_of_line = 0;
						// read scale factor index expressions e.g. 0 (for unscaled) 60 1*2 3*4+1*2*3 etc.
						if (!IO_stream_read_string(this->input_file, "[^\n\r]", &rest_of_line))
						{
							display_message(ERROR_MESSAGE, "EX Reader.  Missing node value label expressions.  %s", this->getFileLocation());
							result = false;
							break;
						}
						char *s = rest_of_line;
						char nextchar;
						const char *token;
						for (int v = 0; v < valueCount; ++v)
						{
							for (int t = 1; t <= termLimit; ++t)
							{
								std::vector<int> scaleFactorIndexes;
								// each term in sum is scaled by product of positive scale factor indexes e.g. 2*3+4, or 0 for unscaled
								int sfCount = 0;
								while (true)
								{
									token = nexttoken(s, "*+", nextchar);
									const int scaleFactorIndex = atoi(token);
									if (!(isIntegerString(token)
										&& (((sfCount == 0) && (scaleFactorIndex == 0))
											|| ((scaleFactorIndex > scaleFactorOffset) && (scaleFactorIndex <= scaleFactorOffset + scaleFactorCount)))))
									{
										display_message(ERROR_MESSAGE, "EX Reader.  Invalid scale factor index '%s' for scale factor set.  %s", token, this->getFileLocation());
										result = false;
										break;
									}
									if (scaleFactorIndex == 0)
									{
										if (nextchar == '*')
										{
											display_message(ERROR_MESSAGE, "EX Reader.  Scale factor index 0 (no scaling) cannot be used in product with other scale factors.  %s", this->getFileLocation());
											result = false;
										}
										break;
									}
									scaleFactorIndexes.push_back(scaleFactorIndex - scaleFactorOffset);
									++sfCount;
									if (nextchar != '*')
										break;
								}
								if (!result)
									break;
								if (t < termCount)
								{
									if (nextchar != '+')
									{
										display_message(ERROR_MESSAGE, "EX Reader.  Require '+' followed by additional term(s).  %s", this->getFileLocation());
										result = false;
										break;
									}
								}
								else if (!(isspace(nextchar) || ((int)'\0' == nextchar)))
								{
									display_message(ERROR_MESSAGE, "EX Reader.  Unexpected character '%c'.  %s", nextchar, this->getFileLocation());
									result = false;
									break;
								}
								if (sfCount > 0)
								{
									if ((sfCount > 1) && (this->exVersion < 2))
									{
										display_message(ERROR_MESSAGE, "EX Reader.  Cannot use product of scale factors with EX Version < 2.  %s",
											this->getFileLocation());
										result = false;
										break;
									}
									usingScaleFactors = true;
									resultCode = eft->setTermScaling(fn + v, t, sfCount, scaleFactorIndexes.data());
									if (resultCode != CMZN_OK)
									{
										display_message(ERROR_MESSAGE, "EX Reader.  Failed to set function %d term %d scaling.  %s",
											fn + v, t, this->getFileLocation());
										result = false;
										break;
									}
								}
							}
							if (!result)
								break;
						}
						if (!result)
							break;
					}
					fn += valueCount;
				}
				if ((sfSet) && (this->exVersion < 2) && (!usingScaleFactors))
				{
					// for EX Version < 2, remove scale factors if none are used
					scaleFactorCount = 0;
					scaleFactorOffset = 0;
					eft->setNumberOfLocalScaleFactors(0);
					sfSet = 0;
				}
				if (sfSet)
				{
					sfSet->addFieldComponent(get_FE_field_name(field), c);
				}
				if (!result)
					break;
				const int packedCount = static_cast<int>(packedNodeIndexes.size());
				if (packedCount < nodeCount)
				{
					if (this->exVersion >= 2)
					{
						display_message(WARNING_MESSAGE, "EX Reader.  Reducing component number of nodes from %d to the number in use, %d.  %s",
							nodeCount, packedCount, this->getFileLocation());
					}
					resultCode = eft->setNumberOfLocalNodes(packedCount);
					if (resultCode != CMZN_OK)
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Failed to reset element field template number of nodes to %d.  %s",
							packedCount, this->getFileLocation());
						result = false;
						break;
					}
				}
				resultCode = eft->sortNodeIndexes(packedNodeIndexes);
				if (resultCode != CMZN_OK)
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Failed to sort element field template node indexes.  %s", this->getFileLocation());
					result = false;
					break;
				}
				componentPackedNodeIndexes[c].swap(packedNodeIndexes);
			} break;
			case CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_ELEMENT:
			{
				this->hasElementValues = true;
				if (elementGridBased)
				{
					KeyValueMap keyValueMap;
					if (!this->readKeyValueMap(keyValueMap))
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Failed to read grid based #xi for each dimension.  %s", this->getFileLocation());
						result = false;
						break;
					}
					int gridNumberInXi[MAXIMUM_ELEMENT_XI_DIMENSIONS];
					// read number of divisions in each xi direction -- on next line
					for (int d = 0; d < dimension; ++d)
					{
						char xiToken[20];
						sprintf(xiToken, "#xi%d", d + 1);
						const char *gridNumberInXiString = keyValueMap.getKeyValue(xiToken);
						if (gridNumberInXiString && isIntegerString(gridNumberInXiString))
						{
							gridNumberInXi[d] = atoi(gridNumberInXiString);
						}
						else
						{
							display_message(WARNING_MESSAGE, "EX Reader.  Missing or invalid %s=NUMBER for grid based component.  %s",
								xiToken, this->getFileLocation());
							result = false;
						}
					}
					if (!result)
						break;
					if (keyValueMap.hasUnusedKeyValues())
					{
						std::string prefix("EX Reader.  Element field ");
						prefix += get_FE_field_name(field);
						prefix += " grid based component ";
						prefix += componentName;
						prefix += ": ";
						keyValueMap.reportUnusedKeyValues(prefix.c_str());
					}
					// don't set grid for element constant
					bool setGrid = false;
					for (int d = 0; d < dimension; ++d)
						if (gridNumberInXi[d] != 0)
						{
							setGrid = true;
							break;
						}
					if (setGrid)
					{
						resultCode = eft->setLegacyGridNumberInXi(gridNumberInXi);
						if (resultCode != CMZN_OK)
						{
							display_message(ERROR_MESSAGE, "EX Reader.  Invalid grid number in xi for field %s at element.  %s",
								get_FE_field_name(field), this->getFileLocation());
							result = false;
						}
					}
				}
			} break;
			case CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_FIELD:
			{
				// nothing more to do
			} break;
			case CMZN_ELEMENTFIELDTEMPLATE_PARAMETER_MAPPING_MODE_INVALID:
			{
				// should never get here as handled above
			} break;
			}
			if (!result)
				break;
		}
		if (!eft->validateAndLock())
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Field %s component %s template is not valid.  %s",
				get_FE_field_name(field), componentName, this->getFileLocation());
			result = false;
			break;
		}
	}
	if (componentName)
		DEALLOCATE(componentName);
	if (result)
	{
		// merge field into FE_region, which may return an existing matching field
		FE_field *mergedField = FE_region_merge_FE_field(this->fe_region, field);
		if (mergedField)
		{
			mergedField->access();
			FE_field::deaccess(&field);
			field = mergedField;
			mergedField = 0;
		}
		else
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Could not merge field %s into region.  %s",
				get_FE_field_name(field), this->getFileLocation());
			result = false;
		}
	}
	if (result)
	{
		bool homogeneous = true;
		for (int c = 1; c < componentCount; ++c)
		{
			if (!(componentEFTs[c]->matches(*(componentEFTs[c - 1]))
				&& (componentPackedNodeIndexes[c] == componentPackedNodeIndexes[c - 1])))
			{
				homogeneous = false;
				break;
			}
		}
		if (homogeneous)
		{
			resultCode = this->elementtemplate->defineField(field, /*all components*/-1, componentEFTs[0]);
			if ((componentPackedNodeIndexes[0].size() > 0) && (resultCode == CMZN_OK))
			{
				resultCode = this->elementtemplate->addLegacyNodeIndexes(field, /*all components*/-1,
					static_cast<int>(componentPackedNodeIndexes[0].size()), componentPackedNodeIndexes[0].data());
			}
		}
		else
		{
			for (int c = 0; c < componentCount; ++c)
			{
				resultCode = this->elementtemplate->defineField(field, c + 1, componentEFTs[c]);
				if ((componentPackedNodeIndexes[c].size() > 0) && (resultCode == CMZN_OK))
				{
					resultCode = this->elementtemplate->addLegacyNodeIndexes(field, c + 1,
						static_cast<int>(componentPackedNodeIndexes[c].size()), componentPackedNodeIndexes[c].data());
				}
				if (resultCode != CMZN_OK)
					break;
			}
		}
		if (resultCode == CMZN_OK)
		{
			this->headerFields.push_back(field);
		}
		else
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Could not define field %s at element.  %s",
				get_FE_field_name(field), this->getFileLocation());
			result = false;
		}
	}
	for (int c = 0; c < componentCount; ++c)
		cmzn_elementfieldtemplate::deaccess(componentEFTs[c]);
	FE_field::deaccess(&field);
	return result;
}

/**
 * Reads an element header description and defines element template from it.
 * Note that the following header is required to return an element template
 * with no fields:
 * #Scale factor sets=0
 * #Nodes=0
 * #Fields=0
 * It is also possible to have no scale factors and no nodes but a field - this
 * would be the case for grid-based fields.
 * @return  True on success, false if failed.
 */
bool EXReader::readElementHeader()
{
	if (!(this->mesh && this->elementShape))
	{
		display_message(ERROR_MESSAGE, "EX Reader.  No mesh or element shape set.  %s", this->getFileLocation());
		return false;
	}
	this->clearHeaderCache(/*clearElementShape*/false);
	if (!this->createElementtemplate())
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Failed to create element template.  %s", this->getFileLocation());
		return false;
	}

	// read in the scale factor set information
	int scaleFactorSetCount;
	if ((1 != IO_stream_scan(this->input_file, "Scale factor sets=%d ", &scaleFactorSetCount))
		|| (scaleFactorSetCount < 0))
	{
		display_message(ERROR_MESSAGE,
			"EX Reader.  Error reading #Scale factor sets.  %s", this->getFileLocation());
		return false;
	}
	int scaleFactorOffset = 0; // accumulate offset of scale factors for set in element
	for (int s = 0; s < scaleFactorSetCount; ++s)
	{
		// read scale factor set name.
		// Historically this was the basis string which may contain spaces and semicolons so need custom delimiters
		char *tmpName = this->readString("[^,\n\r\t]");
		if (!tmpName)
		{
			display_message(ERROR_MESSAGE,
				"EX Reader.  Error reading scale factor set name.  %s", this->getFileLocation());
			return false;
		}
		if (this->exVersion < 2)
		{
			// basis description was used as identifier for scale factor set prior to EX Version 2
			// need to convert scale factor set name into standard basis string
			int *basis_type = FE_basis_string_to_type_array(tmpName);
			FE_basis *basis = FE_region_get_FE_basis_matching_basis_type(this->fe_region, basis_type);
			DEALLOCATE(basis_type);
			DEALLOCATE(tmpName);
			tmpName = FE_basis_get_description_string(basis);
			if (!tmpName)
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Error parsing scale factor set name %s as basis for EX Version < 2.  %s",
					tmpName, this->getFileLocation());
				return false;
			}
		}
		std::string scaleFactorSetName(tmpName);
		DEALLOCATE(tmpName);
		KeyValueMap keyValueMap;
		if (!this->readKeyValueMap(keyValueMap, (int)','))
			return false;
		const char *scaleFactorCountString = keyValueMap.getKeyValue("#Scale factors");
		if (!scaleFactorCountString)
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Missing #Scale factors.  %s", this->getFileLocation());
			return false;
		}
		const int scaleFactorCount = atoi(scaleFactorCountString);
		if (scaleFactorCount <= 0)
		{
			display_message(ERROR_MESSAGE,
				"EX Reader.  Must have positive #Scale factors.  %s", this->getFileLocation());
			return false;
		}
		const char *scaleFactorIdentifiersString = keyValueMap.getKeyValue("identifiers");
		if (keyValueMap.hasUnusedKeyValues())
		{
			std::string prefix("EX Reader.  Scale factor set ");
			prefix += scaleFactorSetName;
			prefix += ": ";
			keyValueMap.reportUnusedKeyValues(prefix.c_str());
		}
		if (0 == this->createScaleFactorSet(scaleFactorSetName.c_str(), scaleFactorCount, scaleFactorOffset, scaleFactorIdentifiersString))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Failed to create scale factor set.  %s", this->getFileLocation());
			return false;
		}
		scaleFactorOffset += scaleFactorCount;
	}

	// read in the node information. This is the number of nodes in the template, indexed from element fields
	int nodeCount;
	if (1 != IO_stream_scan(this->input_file, " #Nodes=%d ", &nodeCount))
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Error reading #Nodes.  %s", this->getFileLocation());
		return false;
	}
	if (CMZN_OK != this->elementtemplate->setNumberOfNodes(nodeCount))
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Failed to set number of nodes to %d.  %s", nodeCount, this->getFileLocation());
		return false;
	}

	// read in the element fields
	int fieldCount = 0;
	if ((1 != IO_stream_scan(this->input_file, " #Fields=%d", &fieldCount)) || (fieldCount < 0))
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Error reading number of fields.  %s", this->getFileLocation());
		return false;
	}
	for (int f = 0; f < fieldCount; ++f)
	{
		if (!this->readElementHeaderField())
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Could not read element field header.  %s", this->getFileLocation());
			return false;
		}
	}
	if (!this->elementtemplate->validate())
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Element field header failed validation.  %s", this->getFileLocation());
		return false;
	}

	// for all scale factor sets, convert [named] field components using them to EFTs
	// warn if same EFT is using different scale factor sets (force use of first one)
	const size_t sfSetCount = this->scaleFactorSets.size();
	std::map<FE_element_field_template*, ScaleFactorSet*> eftSfMap;
	for (size_t s = 0; s < sfSetCount; ++s)
	{
		ScaleFactorSet *sfSet = this->scaleFactorSets[s];
		for (auto fieldIter = sfSet->fieldComponents.begin(); fieldIter != sfSet->fieldComponents.end(); ++fieldIter)
		{
			const std::string &fieldName = fieldIter->first;
			FE_field *field = FE_region_get_FE_field_from_name(this->fe_region, fieldName.c_str());
			const std::vector<int> &componentNumbers = fieldIter->second;
			const size_t limit = componentNumbers.size();
			for (size_t i = 0; i < limit; ++i)
			{
				FE_element_field_template *eft = this->elementtemplate->get_FE_element_template()->getElementfieldtemplate(field, componentNumbers[i]);
				if (!eft)
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Couldn't find element field template for field %s to assign scale factors to.  %s",
						fieldName.c_str(), this->getFileLocation());
					return false;
				}
				auto eftSfIter = eftSfMap.find(eft);
				if (eftSfIter == eftSfMap.end())
				{
					sfSet->addEFT(eft);
				}
				else
				{
					if (eftSfIter->second != sfSet)
					{
						display_message(WARNING_MESSAGE, "EX Reader.  Field %s component %d has a different scale factor set "
							"to other identical element field templates. Forcing use of scale factor set %s.  %s",
							fieldName.c_str(), componentNumbers[i] + 1, eftSfIter->second->name.c_str(), this->getFileLocation());
					}
				}
			}
		}
		if (sfSet->efts.size() == 0)
		{
			display_message(WARNING_MESSAGE, "EX Reader.  Scale factor set %s is not used by any element field components: ignoring.  %s",
				sfSet->name.c_str(), this->getFileLocation());
		}
	}
	return true;
}

/** Read element identifier.
  * In EX Version < 2 this was a triple ELEMENT_NUMBER FACE_NUMBER LINE_NUMBER, where the
  * first non-zero value is used, if not zero.
  * From EX Version 2+ this is a simple integer.
  * @param elementIdentifier  The return identifier. Up to caller to handle negative numbers.
  * @return  True on success, false if failed. */
bool EXReader::readElementIdentifier(DsLabelIdentifier &elementIdentifier)
{
	if (this->exVersion < 2)
	{
		// legacy format: triple of ELEMENT_NUMBER FACE_NUMBER LINE_NUMBER
		DsLabelIdentifier element_num, face_num, line_num;
		if (3 != IO_stream_scan(this->input_file, " %d %d %d", &element_num, &face_num, &line_num))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Error reading legacy element identifier.  %s", this->getFileLocation());
			return false;
		}
		if (element_num)
			elementIdentifier = element_num;
		else if (face_num)
			elementIdentifier = face_num;
		else /* line_num */
			elementIdentifier = line_num;
	}
	else
	{
		if (1 != IO_stream_scan(this->input_file, " %d", &elementIdentifier))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Error reading element identifier.  %s", this->getFileLocation());
			return false;
		}
	}
	return true;
}

/** If field component has element values, read them.
  * @return  True on success, false on failure. */
bool EXReader::readElementFieldComponentValues(DsLabelIndex elementIndex, FE_field *field, int componentNumber)
{
	FE_mesh_field_data *meshFieldData = field->getMeshFieldData(this->mesh);
	const FE_mesh_field_template *mft = meshFieldData->getComponentMeshfieldtemplate(componentNumber);
	const FE_element_field_template *eft = mft->getElementfieldtemplate(elementIndex);
	const int valueCount = eft->getNumberOfElementDOFs();
	if (0 == valueCount)
		return true;
	FE_mesh_field_data::ComponentBase *componentBase = meshFieldData->getComponentBase(componentNumber);
	Value_type valueType = get_FE_field_value_type(field);

	switch (valueType)
	{
	case FE_VALUE_VALUE:
	{
		auto component = static_cast<FE_mesh_field_data::Component<FE_value>*>(componentBase);
		FE_value *values = component->getOrCreateElementValues(elementIndex, valueCount);
		if (!values)
		{
			display_message(ERROR_MESSAGE, "EXReader::readElementFieldComponentValues.  Failed to allocate values.  %s", this->getFileLocation());
			return false;
		}
		for (int v = 0; v < valueCount; ++v)
		{
			if (1 != IO_stream_scan(this->input_file, FE_VALUE_INPUT_STRING, &(values[v])))
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Error reading element/grid FE_value value.  %s", this->getFileLocation());
				return false;
			}
			if (!finite(values[v]))
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Infinity or NAN element value read for element.  %s", this->getFileLocation());
				return false;
			}
		}
	} break;
	case INT_VALUE:
	{
		auto component = static_cast<FE_mesh_field_data::Component<int>*>(componentBase);
		int *values = component->getOrCreateElementValues(elementIndex, valueCount);
		if (!values)
		{
			display_message(ERROR_MESSAGE, "EXReader::readElementFieldComponentValues.  Failed to allocate values.  %s", this->getFileLocation());
			return false;
		}
		for (int v = 0; v < valueCount; ++v)
		{
			if (1 != IO_stream_scan(this->input_file, "%d", &(values[v])))
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Error reading element/grid int value.  %s", this->getFileLocation());
				return false;
			}
		}
	} break;
	default:
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Unsupported element field value type %s.  %s", Value_type_string(valueType), this->getFileLocation());
		return false;
	} break;
	}
	return true;
}

/**
 * Reads element from stream, adding or merging into current mesh.
 * Format:
 * Element: # (EX Version < 2 uses triple # # #  # # #  # # # ...)
 * Faces:
 * # # # ... (EX Version < 2 uses triples # # #  # # #  # # # ...)
 * Values:
 * # # # ...
 * Nodes:
 * # # # ...
 * Scale factors:
 * # # # ...
 *
 * If the element template has nodes, values or scale factors, those
 * sections are mandatory. Faces are optional.
 * Note element and face identifiers are triples in EX Version < 2.
 * @return  On success, ACCESSed element, otherwise 0.
 */
cmzn_element *EXReader::readElement()
{
	char test_string[5];
	if (1 != IO_stream_scan(this->input_file, "ement %1[:] ", test_string)) // "El" has already been read
	{
		display_message(WARNING_MESSAGE, "EX Reader.  Truncated read of Element: token.  %s", this->getFileLocation());
		return 0;
	}
	if (!this->fe_region)
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Region/Group not set before Element: token.  %s", this->getFileLocation());
		return 0;
	}
	if (!((this->mesh) && (this->elementtemplate)))
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Can't read element as no mesh set or no element template defined.  %s", this->getFileLocation());
		return 0;
	}
	DsLabelIdentifier elementIdentifier = DS_LABEL_IDENTIFIER_INVALID;
	if (!this->readElementIdentifier(elementIdentifier))
		return 0;
	if (elementIdentifier < 0)
	{
		display_message(ERROR_MESSAGE, "EX Reader.  Negative element identifier is not permitted.  %s", this->getFileLocation());
		return 0;
	}

	// Note following calls mergeIntoElementEX and createElementEX variants
	// which do not set legacy nodes, since they haven't been read yet!
	cmzn_element *element = this->mesh->findElementByIdentifier(elementIdentifier);
	if (element)
	{
		// existing element may have no shape if automatically created for element:xi location
		// in this case merge will set its shape, otherwise it's an error if existing shape is changed
		const FE_element_shape *elementShape = element->getElementShape();
		if ((elementShape) && (this->elementtemplate->getElementShape() != elementShape))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Element %d redefined with different shape.  %s", elementIdentifier, this->getFileLocation());
			return 0;
		}
		if (CMZN_OK != this->elementtemplate->mergeIntoElementEX(element))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Failed to merge element fields.  %s", this->getFileLocation());
			return 0;
		}
		element->access();
	}
	else
	{
		element = this->elementtemplate->createElementEX(elementIdentifier);
		if (!element)
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Failed to create element.  %s", this->getFileLocation());
			return 0;
		}
	}

	// Faces: (optional)
	if (1 == IO_stream_scan(this->input_file, " Faces %1[:]", test_string))
	{
		const FE_element_shape *elementShape = this->mesh->getElementShape(element->getIndex());
		const int faceCount = FE_element_shape_get_number_of_faces(elementShape);
		FE_mesh *faceMesh = this->mesh->getFaceMesh();
		if ((!faceMesh) && (0 < faceCount))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Failed to find face mesh.  %s", this->getFileLocation());
			cmzn_element::deaccess(element);
			return 0;
		}
		DsLabelIdentifier faceIdentifier = DS_LABEL_IDENTIFIER_INVALID;
		for (int i = 0; i < faceCount; ++i)
		{
			if (!this->readElementIdentifier(faceIdentifier))
			{
				cmzn_element::deaccess(element);
				return 0;
			}
			DsLabelIndex faceIndex = DS_LABEL_INDEX_INVALID;
			if (!((faceIdentifier == -1) // denotes no face
				|| ((faceIdentifier == 0) && (this->exVersion < 2)))) // denoted no face in legacy versions
			{
				faceIndex = faceMesh->findIndexByIdentifier(faceIdentifier);
				if (faceIndex < 0)
				{
					if (faceIdentifier < 0)
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Negative face identifier is not permitted.  %s", this->getFileLocation());
						cmzn_element::deaccess(element);
						return 0;
					}
					// create a face of the expected shape
					FE_element_shape *faceShape = get_FE_element_shape_of_face(elementShape, i, this->fe_region);
					cmzn_element *face = faceMesh->get_or_create_FE_element_with_identifier(faceIdentifier, faceShape);
					if (!face)
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Failed to create face.  %s", this->getFileLocation());
						cmzn_element::deaccess(element);
						return 0;
					}
					faceIndex = face->getIndex();
					cmzn_element::deaccess(face);
				}
			}
			if (CMZN_OK != this->mesh->setElementFace(element->getIndex(), i, faceIndex))
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Failed to set element face.  %s", this->getFileLocation());
				cmzn_element::deaccess(element);
				return 0;
			}
		}
	}

	// Values: element field values if any element-based field components
	const size_t fieldCount = this->headerFields.size();
	if (this->hasElementValues)
	{
		if (1 != IO_stream_scan(this->input_file, " Values %1[:] ", test_string))
		{
			display_message(WARNING_MESSAGE, "EX Reader.  Truncated read of required \" Values :\" token in element.  %s", this->getFileLocation());
			cmzn_element::deaccess(element);
			return 0;
		}
		for (size_t f = 0; f < fieldCount; ++f)
		{
			FE_field *field = this->headerFields[f];
			// only GENERAL_FE_FIELD can store values at elements
			if (GENERAL_FE_FIELD != get_FE_field_FE_field_type(field))
				continue;
			const int componentCount = get_FE_field_number_of_components(field);
			for (int c = 0; c < componentCount; ++c)
			{
				if (!this->readElementFieldComponentValues(element->getIndex(), field, c))
				{
					display_message(WARNING_MESSAGE, "EX Reader.  Failed to read element values.  %s.", this->getFileLocation());
					cmzn_element::deaccess(element);
					return 0;
				}
			}
		}
	}

	// Nodes: if any in element header
	const int nodeCount = this->elementtemplate->getNumberOfNodes();
	if (nodeCount > 0)
	{
		if (1 != IO_stream_scan(this->input_file, " Nodes %1[:]", test_string))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Truncated read of required \" Nodes:\" token in element.  %s", this->getFileLocation());
			cmzn_element::deaccess(element);
			return 0;
		}
		for (int n = 0; n < nodeCount; ++n)
		{
			DsLabelIdentifier nodeIdentifier;
			if (1 != IO_stream_scan(this->input_file, "%d", &nodeIdentifier))
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Error reading node identifier.  %s", this->getFileLocation());
				cmzn_element::deaccess(element);
				return 0;
			}
			cmzn_node *node = 0;
			if (nodeIdentifier >= 0)
			{
				node = this->nodeset->get_or_create_FE_node_with_identifier(nodeIdentifier);
				if (!node)
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Failed to get or create node with identifier %d.  %s", nodeIdentifier, this->getFileLocation());
					cmzn_element::deaccess(element);
					return 0;
				}
			}
			else if (nodeIdentifier < -1)
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Invalid node identifier %d.  %s", nodeIdentifier, this->getFileLocation());
				cmzn_element::deaccess(element);
				return 0;
			}
			const int resultCode = this->elementtemplate->setNode(n + 1, node);
			cmzn_node_destroy(&node);
			if (CMZN_OK != resultCode)
			{
				display_message(ERROR_MESSAGE, "EX Reader.  Failed to set element local node %d.  %s", n + 1, this->getFileLocation());
				cmzn_element::deaccess(element);
				return 0;
			}
		}
		if (CMZN_OK != this->elementtemplate->setLegacyNodesInElement(element))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Failed to set element nodes.  %s", this->getFileLocation());
			cmzn_element::deaccess(element);
			return 0;
		}
	}

	// Scale factors: if any scale factor sets in element header
	const size_t sfSetCount = this->scaleFactorSets.size();
	if (sfSetCount > 0)
	{
		if (1 != IO_stream_scan(this->input_file, " Scale factors %1[:]", test_string))
		{
			display_message(ERROR_MESSAGE, "EX Reader.  Truncated read of required \" Scale factors:\" token in element.  %s", this->getFileLocation());
			cmzn_element::deaccess(element);
			return 0;
		}
		for (size_t ss = 0; ss < sfSetCount; ++ss)
		{
			ScaleFactorSet *sfSet = this->scaleFactorSets[ss];
			// read the values into the sfSet values cache
			const int scaleFactorCount = sfSet->scaleFactorCount;
			FE_value *scaleFactors = sfSet->values.data();
			for (int sf = 0; sf < scaleFactorCount; ++sf)
			{
				if (1 != IO_stream_scan(this->input_file, FE_VALUE_INPUT_STRING, &scaleFactors[sf]))
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Error reading scale factor.  %s", this->getFileLocation());
					cmzn_element::deaccess(element);
					return 0;
				}
				if (!finite(scaleFactors[sf]))
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Infinity or NAN scale factor.  %s", this->getFileLocation());
					cmzn_element::deaccess(element);
					return 0;
				}
			}
			// set scale factors in element for zero or more EFTs using it
			for (auto eftIter = sfSet->efts.begin(); eftIter != sfSet->efts.end(); ++eftIter)
			{
				FE_element_field_template *eft = *eftIter;
				// sanity check
				if (eft->getNumberOfLocalScaleFactors() != sfSet->scaleFactorCount)
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Element field template expects %d scale factors; there are %d in scale factor set.  %s",
						scaleFactorCount, sfSet->scaleFactorCount, this->getFileLocation());
					cmzn_element::deaccess(element);
					return 0;
				}
				FE_mesh_element_field_template_data *meshEftData = this->mesh->getElementfieldtemplateData(eft);
				if (!meshEftData)
				{
					display_message(ERROR_MESSAGE, "EX Reader.  Missing mesh element field template data.  %s", this->getFileLocation());
					cmzn_element::deaccess(element);
					return 0;
				}
				const int result = meshEftData->setElementScaleFactors(element->getIndex(), scaleFactors);
				if (CMZN_OK != result)
				{
					if (CMZN_ERROR_NOT_FOUND == result)
					{
						display_message(WARNING_MESSAGE, "EX Reader.  Can't set element scale factors, "
							"likely reason: using node type scale factors with missing nodes.  %s", this->getFileLocation());
					}
					else
					{
						display_message(ERROR_MESSAGE, "EX Reader.  Failed to set element scale factors.  %s", this->getFileLocation());
						cmzn_element::deaccess(element);
						return 0;
					}
				}
			}
		}
	}

	return element;
}

/**
 * Reads region, group, field, node and element field data in EX format into
 * the supplied region.
 *
 * It is good practice to read the file into a newly created region and check it
 * can be merged into the global region before doing so, otherwise failure to
 * merge incompatible data will leave the global region in a compromised state.
 * Where objects not within the file are referred to, such as nodes in a pure
 * exelem file or elements in embedded element:xi fields, local objects of the
 * correct type are made as placeholders and all checking is left to the merge.
 * Embedding elements are located by a region path starting at the root region
 * in the file; if no path is supplied they are placed in the root region.
 * If objects are repeated in the file, they are merged correctly.
 *
 * @param root_region  The region into which data is read which will be the
 *   root of a region hierarchy when sub-regions and groups are read in.
 * @param input_file  The stream from which EX data is read.
 * @param time_index  If non NULL then the values in this read are assumed to
 *   belong to the specified time.  This means that the nodal values will be
 *   read into an array and the correct index put into the corresponding time
 *   array.
 * @param use_data  Flag, if set indicates nodes are to be read into separate
 *   data regions, otherwise nodes and elements are read normally.
 * @return  1 on success, 0 on failure.
 */
static int read_exregion_file_private(struct cmzn_region *root_region,
	struct IO_stream *input_file, struct FE_import_time_index *time_index,
	int use_data)
{
	if (!(root_region && input_file))
	{
		display_message(ERROR_MESSAGE, "read_exregion_file.  Invalid argument(s)");
		return 0;
	}

	EXReader exReader(input_file, time_index);
	exReader.setUseDataMetaFlag(use_data != 0);
	cmzn_region_begin_hierarchical_change(root_region);
	char first_character_in_token, test_string[5];
	int return_code = 1;
	while (return_code)
	{
		/* get first character in next token */
		IO_stream_scan(input_file, " ");
		int input_result = IO_stream_scan(input_file, "%c", &first_character_in_token);
		if (1 != input_result)
			break;

		switch (first_character_in_token)
		{
			case 'R': /* Region : </path> */
			case 'G': /* Group name : <name> */
			{
				/* Use a %1[:] so that a successful read will return 1 */
				int valid_token = 0;
				if ('R' == first_character_in_token)
				{
					valid_token = IO_stream_scan(input_file,"egion %1[:]", test_string);
				}
				else
				{
					valid_token = IO_stream_scan(input_file,"roup name %1[:]", test_string);
				}
				if (1 != valid_token)
				{
					display_message(ERROR_MESSAGE,
						"EX Reader.  Truncated \'Region :\' or \'Group name :\' token in EX file.  %s", exReader.getFileLocation());
					return_code = 0;
				}
				char *rest_of_line = (char *)NULL;
				char *region_path = (char *)NULL;
				if (return_code)
				{
					/* read the region path */
					if (IO_stream_read_string(input_file, "[^\n\r]", &rest_of_line))
					{
						region_path = rest_of_line;
						/* trim leading and trailing white space */
						while ((' ' == *region_path) || ('\t' == *region_path))
						{
							region_path++;
						}
						char *last_character = region_path+(strlen(region_path)-1);
						while ((last_character > region_path) && ((' ' == *last_character) || ('\t' == *last_character)))
						{
							last_character--;
						}
						*(last_character + 1)='\0';
						return_code = 1;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"EX Reader.  Error reading region path or group name.  %s", exReader.getFileLocation());
						return_code = 0;
					}
				}
				/* get or create region with path, or group with name */
				if (return_code)
				{
					if ('R' == first_character_in_token)
					{
						if (region_path && (CMZN_REGION_PATH_SEPARATOR_CHAR == region_path[0]))
						{
							/* region is the same as read_region if reading into a true region,
							* otherwise it is the parent region of read_region group */
							cmzn_region *region = cmzn_region_find_subregion_at_path(root_region, region_path);
							if (!region)
								region = cmzn_region_create_subregion(root_region, region_path);
							if (!exReader.setRegion(region))
							{
								display_message(ERROR_MESSAGE, "EX Reader.  Could not create or set region \'%s\'.  %s", region_path, exReader.getFileLocation());
								return_code = 0;
							}
							cmzn_region_destroy(&region);
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"Missing \'%c\' at start of region path \'%s\'.  %s",
								CMZN_REGION_PATH_SEPARATOR_CHAR, region_path, exReader.getFileLocation());
							return_code = 0;
						}
					}
					else
					{
						if (0 == exReader.getRegion())
						{
							if (!exReader.setRegion(root_region))
							{
								display_message(ERROR_MESSAGE, "EX Reader.  Could not set root region.  %s", exReader.getFileLocation());
								return_code = 0;
							}
						}
						if (return_code)
						{
							if (!exReader.setFieldGroupOfName(region_path))
							{
								display_message(ERROR_MESSAGE,
									"EX Reader.  Could not create group \'%s\'.  %s", region_path, exReader.getFileLocation());
								return_code = 0;
							}
						}
					}
				}
				DEALLOCATE(rest_of_line);
			} break;
			case 'S': /* Shape */
			{
				if (!exReader.readElementShape())
					return_code = 0;
			} break;
			case '!': /* !# directive, otherwise ! Comment ignored to end of line */
			{
				if (!exReader.readCommentOrDirective())
					return_code = 0;
			} break;
			case '#': /* #Scale factor sets, #Nodes, or #Fields */
			{
				if (exReader.getMesh())
				{
					if (!exReader.readElementHeader())
						return_code = 0;
				}
				else if (exReader.getNodeset())
				{
					if (!exReader.readNodeHeader())
						return_code = 0;
				}
				else
				{
					display_message(ERROR_MESSAGE, "Region/Group not set before field header.  %s", exReader.getFileLocation());
					return_code = 0;
				}
			} break;
			case 'N': /* Node */
			{
				cmzn_node *node = exReader.readNode();
				if (node)
				{
					cmzn_nodeset_group *nodesetGroup = exReader.getNodesetGroup();
					if (nodesetGroup)
						cmzn_nodeset_group_add_node(nodesetGroup, node);
					cmzn_node::deaccess(node);
				}
				else
				{
					display_message(ERROR_MESSAGE, "read_exregion_file.  Error reading node");
					return_code = 0;
				}
			} break;
			case 'E': /* Element or EX Version */
			{
				const int next_char = exReader.readNextNonSpaceChar();
				if (next_char == (int)'l')
				{
					cmzn_element *element = exReader.readElement();
					if (element)
					{
						cmzn_mesh_group *meshGroup = exReader.getMeshGroup();
						if (meshGroup)
							cmzn_mesh_group_add_element(meshGroup, element);
						cmzn_element::deaccess(element);
					}
					else
					{
						display_message(ERROR_MESSAGE, "read_exregion_file.  Error reading element");
						return_code = 0;
					}
				}
				else if (next_char == (int)'X')
				{
					if (!exReader.readEXVersion())
					{
						return_code = 0;
					}
				}
				else
				{
					char *temp_string = 0;
					IO_stream_read_string(input_file, "[^\n\r]", &temp_string);
					display_message(ERROR_MESSAGE,
						"Invalid token \'%c%s\' in EX node/element file.  %s",
						first_character_in_token, temp_string ? temp_string : "", exReader.getFileLocation());
					DEALLOCATE(temp_string);
					return_code = 0;
				}
			} break;
			case 'V': /* Values */
			{
				if (!exReader.readFieldValues())
					return_code = 0;
			} break;
			default:
			{
				char *temp_string = 0;
				IO_stream_read_string(input_file, "[^\n\r]", &temp_string);
				display_message(ERROR_MESSAGE,
					"Invalid token \'%c%s\' in EX node/element file.  %s",
					first_character_in_token, temp_string ? temp_string : "", exReader.getFileLocation());
				DEALLOCATE(temp_string);
				return_code = 0;
			} break;
		} /* switch (first_character_in_token) */
	}
	cmzn_region_end_hierarchical_change(root_region);
	return (return_code);
}

/*
Global functions
----------------
*/

int read_exregion_file(struct cmzn_region *region,
	struct IO_stream *input_file, struct FE_import_time_index *time_index)
{
	return read_exregion_file_private(region, input_file, time_index,
		/*use_data*/0);
}

int read_exdata_file(struct cmzn_region *region,
	struct IO_stream *input_file, struct FE_import_time_index *time_index)
{
	return read_exregion_file_private(region, input_file, time_index,
		/*use_data*/1);
}

int read_exregion_file_of_name(struct cmzn_region *region, const char *file_name,
	struct IO_stream_package *io_stream_package,
	struct FE_import_time_index *time_index, int useData,
	enum cmzn_streaminformation_data_compression_type data_compression_type)
/*******************************************************************************
LAST MODIFIED : 23 May 2008

DESCRIPTION :
Version of read_exregion_file that opens and closes file <file_name>.
Up to the calling function to check and merge the returned cmiss_region.
==============================================================================*/
{
	int return_code;
	struct IO_stream *input_file;

	ENTER(read_exregion_file_of_name);
	return_code = 0;
	if (region && file_name)
	{
		input_file = CREATE(IO_stream)(io_stream_package);
		if (IO_stream_open_for_read_compression_specified(input_file, file_name, data_compression_type))
		{
			return_code = read_exregion_file_private(region, input_file, time_index,
				useData);
			IO_stream_close(input_file);
		}
		else
		{
			display_message(ERROR_MESSAGE, "Could not open exregion file: %s",
				file_name);
		}
		DESTROY(IO_stream)(&input_file);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"read_exregion_file_of_name.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* read_exregion_file_of_name */
