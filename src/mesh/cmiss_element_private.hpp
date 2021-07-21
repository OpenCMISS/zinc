/***************************************************************************//**
 * FILE : cmiss_element_private.hpp
 *
 * Private header file of cmzn_element, finite element meshes.
 *
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CMZN_ELEMENT_PRIVATE_HPP)
#define CMZN_ELEMENT_PRIVATE_HPP

#include "opencmiss/zinc/element.h"
#include "opencmiss/zinc/region.h"
#include "opencmiss/zinc/fieldsubobjectgroup.h"
#include "datastore/labelschangelog.hpp"
#include "finite_element/finite_element_region.h"
#include "finite_element/finite_element_mesh.hpp"

struct FE_basis;
class FE_mesh;
struct FE_region;

class LegacyElementFieldData;

/** External handle to element basis description; convertable to FE_basis */
struct cmzn_elementbasis
{
private:
	FE_region *fe_region; // needed to get basis manager
	int dimension;
	cmzn_elementbasis_function_type *function_types;
	int access_count;

	cmzn_elementbasis(FE_region *fe_region, int mesh_dimension,
		cmzn_elementbasis_function_type function_type);

	~cmzn_elementbasis();

public:
	static cmzn_elementbasis* create(FE_region *fe_region, int mesh_dimension,
		cmzn_elementbasis_function_type function_type);

	static cmzn_elementbasis* create(FE_region *fe_region, FE_basis *basis);

	cmzn_elementbasis_id access()
	{
		++access_count;
		return this;
	}

	static int deaccess(cmzn_elementbasis_id &basis)
	{
		if (!basis)
			return CMZN_ERROR_ARGUMENT;
		--(basis->access_count);
		if (basis->access_count <= 0)
			delete basis;
		basis = 0;
		return CMZN_OK;
	}

	int getDimension() const
	{
		return dimension;
	}

	/** @return  number of dimension using supplied function_type */
	int getDimensionsUsingFunction(cmzn_elementbasis_function_type function_type) const;

	/** @return  1 if all function types set & at least 2 chart components linked for each simplex basis */
	int isValid() const;

	/** @return  Accessed FE_basis, or NULL on error */
	FE_basis *getFeBasis() const;

	enum cmzn_elementbasis_function_type getFunctionType(int chart_component) const;

	int setFunctionType(int chart_component, cmzn_elementbasis_function_type function_type);

	int getNumberOfNodes() const;

	int getNumberOfFunctions() const;

	int getNumberOfFunctionsPerNode(int basisNodeIndex) const;

};


/** External element template object handling legacy node mappings */
struct cmzn_elementtemplate
{
private:
	FE_element_template *fe_element_template; // internal implementation
	int legacyNodesCount;
	cmzn_node **legacyNodes;
	std::vector<LegacyElementFieldData*> legacyFieldDataList;
	int access_count;

	cmzn_elementtemplate(FE_mesh *fe_mesh_in);

	~cmzn_elementtemplate();

	inline void beginChange()
	{
		FE_region_begin_change(this->getMesh()->get_FE_region());
	}

	inline void endChange()
	{
		FE_region_end_change(this->getMesh()->get_FE_region());
	}

	void clearLegacyNodes();

	void clearLegacyElementFieldData(FE_field *fe_field);

	LegacyElementFieldData *getLegacyElementFieldData(FE_field *fe_field);

	LegacyElementFieldData *getOrCreateLegacyElementFieldData(FE_field *fe_field);

public:

	int setLegacyNodesInElement(cmzn_element *element);

	static cmzn_elementtemplate *create(FE_mesh *fe_mesh_in)
	{
		if (fe_mesh_in)
			return new cmzn_elementtemplate(fe_mesh_in);
		return 0;
	}

	cmzn_elementtemplate_id access()
	{
		++access_count;
		return this;
	}

	static int deaccess(cmzn_elementtemplate_id &element_template)
	{
		if (!element_template)
			return CMZN_ERROR_ARGUMENT;
		--(element_template->access_count);
		if (element_template->access_count <= 0)
			delete element_template;
		element_template = 0;
		return CMZN_OK;
	}

	/** @return  Non-accessed shape */
	FE_element_shape *getElementShape() const
	{
		return this->fe_element_template->getElementShape();
	}

	int setElementShape(FE_element_shape *elementShape)
	{
		return this->fe_element_template->setElementShape(elementShape);
	}

	cmzn_element_shape_type getShapeType() const
	{
		return FE_element_shape_get_simple_type(this->fe_element_template->getElementShape());
	}

	int setElementShapeType(cmzn_element_shape_type shapeTypeIn);

	FE_mesh *getMesh() const
	{
		return this->fe_element_template->getMesh();
	}

	int getNumberOfNodes() const
	{
		return this->legacyNodesCount;
	}

	int setNumberOfNodes(int legacyNodesCountIn);

	int defineField(FE_field *field, int componentNumber, cmzn_elementfieldtemplate *eft);

	int defineField(cmzn_field_id field, int componentNumber, cmzn_elementfieldtemplate *eft);

	/** Append legacy node indexes giving index in element template of node for each EFT local node.
	  * @param componentNumber  -1 for all components, or positive for single component. */
	int addLegacyNodeIndexes(FE_field *field, int componentNumber, int nodeIndexesCount,
		const int *nodeIndexes);

	int defineFieldSimpleNodal(cmzn_field_id field,
		int componentNumber, cmzn_elementbasis_id elementbasis, int nodeIndexesCount,
		const int *nodeIndexes);

	int defineFieldElementConstant(cmzn_field_id field, int componentNumber);

	/** @param componentNumber  -1 for all components, or positive for single component
	  * \note DEPRECATED; only expected to work with defineFieldSimpleNodal */
	int setMapNodeValueLabel(cmzn_field_id field, int componentNumber,
		int basisNodeIndex, int nodeFunctionIndex, cmzn_node_value_label nodeValueLabel);

	/** @param componentNumber  -1 for all components, or positive for single component
	  * \note DEPRECATED; only expected to work with defineFieldSimpleNodal */
	int setMapNodeVersion(cmzn_field_id field, int componentNumber,
		int basisNodeIndex, int nodeFunctionIndex, int versionNumber);

	bool validate() const
	{
		return this->fe_element_template->validate();
	}

	/** @param local_node_index  Index from 1 to legacy nodes count.
	  * @return  Non-accessed node, or 0 if invalid index or no node at index. */
	cmzn_node_id getNode(int local_node_index);

	/** @param local_node_index  Index from 1 to legacy nodes count. */
	int setNode(int local_node_index, cmzn_node_id node);

	int removeField(cmzn_field_id field);

	int undefineField(cmzn_field_id field);

	cmzn_element *createElement(int identifier);

	/** Variant for EX reader which assumes template has already been validated,
	  * does not set legacy nodes and does not cache changes as assumed on */
	cmzn_element *createElementEX(int identifier)
	{
		return this->getMesh()->create_FE_element(identifier, this->fe_element_template);
	}

	int mergeIntoElement(cmzn_element *element);

	/** Variant for EX reader which assumes template has already been validated,
	  * does not set legacy nodes and does not cache changes as assumed on */
	int mergeIntoElementEX(cmzn_element *element)
	{
		return this->getMesh()->merge_FE_element_template(element, this->fe_element_template);
	}

	FE_element_template *get_FE_element_template()
	{
		return this->fe_element_template;
	}

};

/** Create external cmzn_mesh with access_count 1 from internal FE_mesh */
cmzn_mesh *cmzn_mesh_create(FE_mesh *fe_mesh);

/***************************************************************************//**
 * Ensures all faces of the supplied element are in this mesh_group.
 * Candidate for external API.
 *
 * @param mesh_group  The mesh group to add faces to. Must be of dimension 1
 * less than that of element and be a subgroup for the master mesh expected to
 * own the element's faces.
 * @param element  The element whose faces are to be added.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
int cmzn_mesh_group_add_element_faces(cmzn_mesh_group_id mesh_group,
	cmzn_element_id element);

/***************************************************************************//**
 * Ensures all faces of the supplied element are not in this mesh_group.
 * Candidate for external API.
 *
 * @param mesh_group  The mesh group to remove faces from. Must be of dimension
 * 1 less than that of element and be a subgroup for the master mesh expected
 * to own the element's faces.
 * @param element  The element whose faces are to be removed.
 * @return  Status CMZN_OK on success, any other value on failure.
 */
int cmzn_mesh_group_remove_element_faces(cmzn_mesh_group_id mesh_group,
	cmzn_element_id element);

/** Internal use only
 * @return non-accessed FE_mesh for this mesh.
 */
FE_mesh *cmzn_mesh_get_FE_mesh_internal(cmzn_mesh_id mesh);

/** Internal use only
 * @return non-accessed FE_region for this mesh.
 */
FE_region *cmzn_mesh_get_FE_region_internal(cmzn_mesh_id mesh);

/** Internal use only.
 * @return non-accessed region for this mesh.
 */
cmzn_region_id cmzn_mesh_get_region_internal(cmzn_mesh_id mesh);

/** Internal use only.
 * @return non-accessed element group field for this mesh, if any.
 */
cmzn_field_element_group *cmzn_mesh_get_element_group_field_internal(cmzn_mesh_id mesh);

struct cmzn_meshchanges
{
private:
	cmzn_fieldmoduleevent *event; // accessed
	DsLabelsChangeLog *changeLog; // Accessed from object obtained from correct mesh
	int access_count;

	cmzn_meshchanges(cmzn_fieldmoduleevent *eventIn, cmzn_mesh *meshIn);
	~cmzn_meshchanges();

public:

	static cmzn_meshchanges *create(cmzn_fieldmoduleevent *eventIn, cmzn_mesh *meshIn);

	cmzn_meshchanges *access()
	{
		++(this->access_count);
		return this;
	}

	static int deaccess(cmzn_meshchanges* &meshchanges);

	/** Also checks related changes to parent elements and nodes */
	cmzn_element_change_flags getElementChangeFlags(cmzn_element *element);

	int getNumberOfChanges()
	{
		if (this->changeLog->isAllChange())
			return -1;
		return this->changeLog->getChangeCount();
	}

	cmzn_element_change_flags getSummaryElementChangeFlags()
	{
		return this->changeLog->getChangeSummary();
	}
};

/**
 * If the name is of the form GROUP_NAME.MESH_NAME. Create a mesh group.
 * For internal use in command migration only.
 *
 * @param field_module  The field module the mesh belongs to.
 * @param name  The name of the mesh: GROUP_NAME.{mesh1d|mesh2d|mesh3d}.
 * @return  Handle to the mesh, or NULL if error, name already in use or no
 * such mesh name.
 */
cmzn_mesh_group_id cmzn_fieldmodule_create_mesh_group_from_name_internal(
	cmzn_fieldmodule_id field_module, const char *mesh_group_name);

/**
 * Internal function for getting the FE_basis for elementbasis in its
 * current state.
 * @return  Non-accessed FE_basis on success, 0 on failure. Note FE_basis
 * should remain in existence until shutdown.
 */
FE_basis *cmzn_elementbasis_get_FE_basis(cmzn_elementbasis_id elementbasis);

#endif /* !defined (CMZN_ELEMENT_PRIVATE_HPP) */
