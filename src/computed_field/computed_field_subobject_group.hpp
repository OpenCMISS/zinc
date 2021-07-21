/***************************************************************************//**
 * FILE : computed_field_subobject_group.hpp
 *
 * Implements region sub object groups, e.g. node group, element group.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMPUTED_FIELD_SUBOBJECT_GROUP_HPP)
#define COMPUTED_FIELD_SUBOBJECT_GROUP_HPP
#include <stdlib.h>
#include "opencmiss/zinc/fieldsubobjectgroup.h"
#include "computed_field/computed_field.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_mesh.hpp"
#include "finite_element/finite_element_nodeset.hpp"
#include "finite_element/finite_element_region.h"
#include "computed_field/computed_field_group_base.hpp"
#include "computed_field/computed_field_group.hpp"
#include "computed_field/computed_field_private.hpp"
#include "general/cmiss_set.hpp"
#include "general/debug.h"
#include "region/cmiss_region.hpp"
#include "general/message.h"
#include <map>
#include <iterator>

/***************************************************************************//**
 * Change details for simple object groups where a single change status is
 * sufficient.
 */
struct cmzn_field_subobject_group_change_detail : public cmzn_field_group_base_change_detail
{
private:
	int changeSummary;

public:
	cmzn_field_subobject_group_change_detail() :
		changeSummary(CMZN_FIELD_GROUP_CHANGE_NONE)
	{
	}

	virtual void clear()
	{
		changeSummary = CMZN_FIELD_GROUP_CHANGE_NONE;
	}

	virtual int getChangeSummary() const
	{
		return changeSummary;
	}

	/** Inform object(s) have been added */
	void changeAdd()
	{
		this->changeSummary |= CMZN_FIELD_GROUP_CHANGE_ADD;
	}

	/** Inform object(s) have been removed (clear handled separately) */
	void changeRemove()
	{
		this->changeSummary |= CMZN_FIELD_GROUP_CHANGE_REMOVE;
	}

};

class Computed_field_subobject_group : public Computed_field_group_base
{
protected:
	Computed_field_group *ownerGroup; // not accessed

public:

	Computed_field_subobject_group() :
		Computed_field_group_base(),
		ownerGroup(0)
	{
	}

	virtual ~Computed_field_subobject_group();

	const char* get_type_string()
	{
		return ("sub_group_object");
	}

	virtual int isIdentifierInList(int identifier) = 0;
	virtual int containsIndex(DsLabelIndex index) = 0;

	bool check_dependency_for_group_special()
	{
		if (field->manager_change_status & MANAGER_CHANGE_RESULT(Computed_field))
			return true;
		else if (field->manager_change_status & MANAGER_CHANGE_ADD(Computed_field))
		{
			const cmzn_field_subobject_group_change_detail *change_detail =
				dynamic_cast<const cmzn_field_subobject_group_change_detail *>(get_change_detail());
			const int changeSummary = change_detail->getChangeSummary();
			if (changeSummary & CMZN_FIELD_GROUP_CHANGE_ADD)
				return true;
		}
		return false;
	}

	// Set for subobject groups which are managed by a Computed_field_group.
	void setOwnerGroup(Computed_field_group *ownerGroupIn)
	{
		this->ownerGroup = ownerGroupIn;
	}

	inline cmzn_field_group_subelement_handling_mode getSubobjectHandlingMode() const
	{
		if (this->ownerGroup)
			return this->ownerGroup->getSubelementHandlingMode();
		return CMZN_FIELD_GROUP_SUBELEMENT_HANDLING_MODE_NONE;
	}

};

	template <typename T>
	class Computed_field_sub_group_object : public Computed_field_subobject_group
	{
	public:

		Computed_field_sub_group_object() :
			Computed_field_subobject_group(),
			object_map()
		{
			object_pos = object_map.begin();
		}

		~Computed_field_sub_group_object()
		{
		}

		inline int add_object(int identifier, T object)
		{
			if (object_map.insert(std::make_pair(identifier,object)).second)
			{
				change_detail.changeAdd();
				update();
				return 1;
			}
			return 0;
		};

		inline int remove_object(int identifier)
		{
			if (object_map.find(identifier) != object_map.end())
			{
				object_map.erase(identifier);
				change_detail.changeRemove();
				update();
				return 1;
			}
			return 0;
		};

		inline T get_object(int identifier)
		{
			T return_object = NULL;
			if (object_map.find(identifier) != object_map.end())
				return_object = object_map.find(identifier)->second;

			return return_object;
		}

		virtual int clear()
		{
			if (object_map.size())
			{
				object_map.clear();
				change_detail.changeRemove();
				update();
			}
			return 1;
		};

		int get_object_selected(int identifier,T object)
		{
			USE_PARAMETER(object);
			int return_code = 0;
			if (object_map.find(identifier) != object_map.end() &&
				object_map.find(identifier)->second == object)
			{
				return_code = 1;
			}
			else
			{
				return_code = 0;
			}

			return (return_code);
		};

		virtual bool isEmpty() const
		{
			return object_map.empty();
		}

		virtual int isIdentifierInList(int identifier)
		{
			return (!(object_map.empty()) && (object_map.find(identifier) != object_map.end()));
		}

		T getFirstObject()
		{
			T return_object = NULL;
			object_pos = object_map.begin();
			if (object_pos != object_map.end())
			{
				return_object = object_pos->second;
			}
			return return_object;
		}

		T getNextObject()
		{
			T return_object = NULL;
			if (object_pos != object_map.end())
			{
				object_pos++;
				if (object_pos != object_map.end())
				{
					return_object = object_pos->second;
				}
			}
			return return_object;
		}

		virtual cmzn_field_change_detail *extract_change_detail()
		{
			if (this->change_detail.getChangeSummary() == CMZN_FIELD_GROUP_CHANGE_NONE)
				return 0;
			cmzn_field_subobject_group_change_detail *prior_change_detail =
				new cmzn_field_subobject_group_change_detail(change_detail);
			change_detail.clear();
			return prior_change_detail;
		}

		virtual const cmzn_field_change_detail *get_change_detail() const
		{
			return &change_detail;
		}

	private:

		std::map<int, T> object_map;
		cmzn_field_subobject_group_change_detail change_detail;
		typename std::map<int, T>::iterator object_pos;

		Computed_field_core* copy()
		{
			Computed_field_sub_group_object *core = new Computed_field_sub_group_object();
			core->object_map = this->object_map;
			return (core);
		};

		int compare(Computed_field_core* other_field)
		{
			int return_code;

			ENTER(Computed_field_sub_group_object::compare);
			if (field && dynamic_cast<Computed_field_sub_group_object<T>*>(other_field))
			{
				return_code = 1;
			}
			else
			{
				return_code = 0;
			}
			LEAVE;

			return (return_code);
		}

		int list()
		{
			return 1;
		};

		inline void update()
		{
			this->field->setChanged();
		}

	};

	class Computed_field_element_group : public Computed_field_subobject_group
	{
	private:
		FE_mesh *fe_mesh;
		DsLabelsGroup *labelsGroup;
		cmzn_field_subobject_group_change_detail change_detail;

		Computed_field_element_group(FE_mesh *fe_mesh_in, DsLabelsGroup *labelsGroupIn) :
			Computed_field_subobject_group(),
			fe_mesh(fe_mesh_in->access()),
			labelsGroup(cmzn::Access(labelsGroupIn))
		{
		}

		~Computed_field_element_group()
		{
			cmzn::Deaccess(this->labelsGroup);
			FE_mesh::deaccess(this->fe_mesh);
		}

	public:

		static Computed_field_element_group *create(FE_mesh *fe_mesh_in);

		/** @return  Non-accessed FE_mesh */
		FE_mesh *get_fe_mesh() const
		{
			return this->fe_mesh;
		}

		DsLabelsGroup& getLabelsGroup() const
		{
			return *(this->labelsGroup);
		}

		int addObject(cmzn_element *object);

		// add object with identifier ranges from first to last, inclusive
		int addObjectsInIdentifierRange(DsLabelIdentifier first, DsLabelIdentifier last);

		/** add any elements from master mesh for which conditional_field is true */
		int addElementsConditional(cmzn_field_id conditional_field);

		int removeObject(cmzn_element *object);

		/** remove all elements for which conditional_field is true */
		int removeElementsConditional(cmzn_field_id conditional_field);

		virtual int clear();

		bool containsObject(cmzn_element *object)
		{
			return this->isElementCompatible(object) &&
				this->labelsGroup->hasIndex(get_FE_element_index(object));
		};

		cmzn_elementiterator_id createElementiterator()
		{
			return this->fe_mesh->createElementiterator(this->labelsGroup);
		}

		/** @return  Non-accessed element with that identifier, or 0 if none */
		inline cmzn_element_id findElementByIdentifier(int identifier)
		{
			const DsLabelIndex index = this->fe_mesh->findIndexByIdentifier(identifier);
			if (this->containsIndex(index))
				return this->fe_mesh->getElement(index);
			return 0;
		}

		int getSize()
		{
			return this->labelsGroup->getSize();
		}

		virtual bool isEmpty() const
		{
			return this->labelsGroup->getSize() == 0;
		}

		virtual int isIdentifierInList(int identifier)
		{
			const DsLabelIndex index = this->fe_mesh->findIndexByIdentifier(identifier);
			return this->labelsGroup->hasIndex(index);
		}

		virtual int containsIndex(DsLabelIndex index)
		{
			return this->labelsGroup->hasIndex(index);
		}

		virtual cmzn_field_change_detail *extract_change_detail()
		{
			if (this->change_detail.getChangeSummary() == CMZN_FIELD_GROUP_CHANGE_NONE)
				return 0;
			cmzn_field_subobject_group_change_detail *prior_change_detail =
				new cmzn_field_subobject_group_change_detail(change_detail);
			change_detail.clear();
			return prior_change_detail;
		}

		virtual const cmzn_field_change_detail *get_change_detail() const
		{
			return &change_detail;
		}

		void write_btree_statistics() const;

		/** ensure parent element's faces are in element group */
		int addElementFaces(cmzn_element_id parent);

		/** ensure parent element's faces are not in element group */
		int removeElementFaces(cmzn_element_id parent);

	private:

		/** Adds faces and nodes of element to related subobject groups.
		 * only call with this->ownerGroup set, and between begin/end change */
		int addSubelements(cmzn_element_id element);

		/** Removes faces and nodes of element from related subobject groups, but
		 * only if not used by peers.
		 * only call with this->ownerGroup set, and between begin/end change */
		int removeSubelements(cmzn_element_id element);

		/** Removes faces and nodes of elements in list from related subobject
		 * groups, but only if not used by peers.
		 * only call with this->ownerGroup set, and between begin/end change */
		int removeSubelementsList(DsLabelsGroup &removedlabelsGroup);

		/** Adds faces of parent element to element group, and their faces to related group
		 * recursively. Only call with this->ownerGroup set, and between begin/end change */
		int addElementFacesRecursive(FE_mesh& parentMesh, DsLabelIndex parentIndex);

		/** Removes faces of parent element from element group, and their faces from related group
		 * recursively. Only call with this->ownerGroup set, and between begin/end change */
		int removeElementFacesRecursive(Computed_field_element_group& parentElementGroup, DsLabelIndex parentIndex);

		Computed_field_core* copy()
		{
			return Computed_field_element_group::create(this->fe_mesh);
		};

		int compare(Computed_field_core* other_field)
		{
			if (field && dynamic_cast<Computed_field_element_group*>(other_field))
				return 1;
			return 0;
		}

		virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

		virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
		{
			inValueCache.getDerivativeValueCache(fieldDerivative)->zeroValues();
			return 1;
		}

		int list()
		{
			return 1;
		};

		inline void update()
		{
			this->field->setChanged();
		}

		virtual int check_dependency();

		bool isElementCompatible(cmzn_element_id element)
		{
			return this->fe_mesh->containsElement(element);
		}

		bool isParentElementCompatible(cmzn_element_id element)
		{
			FE_mesh *parent_fe_mesh = this->fe_mesh->getParentMesh();
			if (parent_fe_mesh)
				return parent_fe_mesh->containsElement(element);
			return false;
		}

		/**
		 * @param isEmptyGroup  True if there is a group, but the element group is empty.
		 * @return  Non-empty element group, or 0 if not a group or empty.
		 */
		Computed_field_element_group *getConditionalElementGroup(cmzn_field *conditionalField, bool &isEmptyGroup) const;

		void invalidateIterators()
		{
			this->labelsGroup->invalidateLabelIterators();
		}
	};

	class Computed_field_node_group : public Computed_field_subobject_group
	{
	private:
		FE_nodeset *fe_nodeset;
		DsLabelsGroup *labelsGroup;
		cmzn_field_subobject_group_change_detail change_detail;

		Computed_field_node_group(FE_nodeset *fe_nodeset_in, DsLabelsGroup *labelsGroupIn) :
			Computed_field_subobject_group(),
			fe_nodeset(fe_nodeset_in->access()),
			labelsGroup(cmzn::Access(labelsGroupIn))
		{
		}

		~Computed_field_node_group()
		{
			cmzn::Deaccess(this->labelsGroup);
			FE_nodeset::deaccess(this->fe_nodeset);
		}

	public:

		static Computed_field_node_group *create(FE_nodeset *fe_nodeset_in);

		/** @return  Non-accessed FE_nodeset */
		FE_nodeset *get_fe_nodeset() const
		{
			return this->fe_nodeset;
		}

		DsLabelsGroup& getLabelsGroup() const
		{
			return *(this->labelsGroup);
		}

		int addObject(cmzn_node *object);

		int addObjectsInIdentifierRange(DsLabelIdentifier first, DsLabelIdentifier last);

		/** add any nodes from master nodeset for which conditional_field is true */
		int addNodesConditional(cmzn_field_id conditional_field);

		int removeObject(cmzn_node *object);

		/** remove all nodes for which conditional_field is true */
		int removeNodesConditional(cmzn_field_id conditional_field);

		int removeNodesInLabelsGroup(DsLabelsGroup &removeNodeLabelsGroup);

		virtual int clear();

		bool containsObject(cmzn_node *object)
		{
			return this->isNodeCompatible(object) &&
				this->labelsGroup->hasIndex(object->getIndex());
		};

		cmzn_nodeiterator_id createNodeiterator()
		{
			return this->fe_nodeset->createNodeiterator(this->labelsGroup);
		}

		/** @return  non-accessed node with that identifier, or 0 if none */
		inline cmzn_node_id findNodeByIdentifier(int identifier)
		{
			const DsLabelIndex index = this->fe_nodeset->findIndexByIdentifier(identifier);
			if (this->containsIndex(index))
				return this->fe_nodeset->getNode(index);
			return 0;
		}

		int getSize()
		{
			return this->labelsGroup->getSize();
		}

		virtual bool isEmpty() const
		{
			return this->labelsGroup->getSize() == 0;
		}

		virtual int isIdentifierInList(int identifier)
		{
			const DsLabelIndex index = this->fe_nodeset->findIndexByIdentifier(identifier);
			return this->labelsGroup->hasIndex(index);
		}

		virtual int containsIndex(DsLabelIndex index)
		{
			return this->labelsGroup->hasIndex(index);
		}

		virtual cmzn_field_change_detail *extract_change_detail()
		{
			if (this->change_detail.getChangeSummary() == CMZN_FIELD_GROUP_CHANGE_NONE)
				return 0;
			cmzn_field_subobject_group_change_detail *prior_change_detail =
				new cmzn_field_subobject_group_change_detail(change_detail);
			change_detail.clear();
			return prior_change_detail;
		}

		virtual const cmzn_field_change_detail *get_change_detail() const
		{
			return &change_detail;
		}

		void write_btree_statistics() const;

		/** ensure element's nodes are in node group */
		int addElementNodes(cmzn_element_id element);

		/** ensure element's nodes are not in node group */
		int removeElementNodes(cmzn_element_id element);

		DsLabelsGroup *createRelatedNodeLabelsGroup()
		{
			return this->fe_nodeset->createLabelsGroup();
		}

	private:

		Computed_field_core* copy()
		{
			return Computed_field_node_group::create(this->fe_nodeset);
		};

		int compare(Computed_field_core* other_field)
		{
			if (field && dynamic_cast<Computed_field_node_group*>(other_field))
				return 1;
			return 0;
		}

		virtual int evaluate(cmzn_fieldcache& cache, FieldValueCache& inValueCache);

		virtual int evaluateDerivative(cmzn_fieldcache& cache, RealFieldValueCache& inValueCache, const FieldDerivative& fieldDerivative)
		{
			inValueCache.getDerivativeValueCache(fieldDerivative)->zeroValues();
			return 1;
		}

		int list()
		{
			return 1;
		};

		inline void update()
		{
			this->field->setChanged();
		}

		static int FE_node_is_not_in_FE_nodeset(cmzn_node *node, void *fe_nodeset_void)
		{
			return !(reinterpret_cast<FE_nodeset*>(fe_nodeset_void)->containsNode(node));
		}

		virtual int check_dependency();

		bool isNodeCompatible(cmzn_node_id node)
		{
			return this->fe_nodeset->containsNode(node);
		}

		bool isParentElementCompatible(cmzn_element_id element)
		{
			if ((element) && element->getMesh())
				return (this->fe_nodeset->get_FE_region() == element->getMesh()->get_FE_region());
			return false;
		}

		/**
		 * @param isEmptyGroup  True if there is a group, but the node group is empty.
		 * @return  Non-empty node group, or 0 if not a group or empty.
		 */
		Computed_field_node_group *getConditionalNodeGroup(cmzn_field *conditionalField, bool &isEmptyGroup) const;

		void invalidateIterators()
		{
			this->labelsGroup->invalidateLabelIterators();
		}

	};

template <typename ObjectType, typename FieldType>
Computed_field_sub_group_object<ObjectType> *Computed_field_sub_group_object_core_cast(
	FieldType object_group_field)
 {
	return (static_cast<Computed_field_sub_group_object<ObjectType>*>(
		reinterpret_cast<Computed_field*>(object_group_field)->core));
 }

inline Computed_field_element_group *Computed_field_element_group_core_cast(
	cmzn_field_element_group_id object_group_field)
{
	return (static_cast<Computed_field_element_group *>(
		reinterpret_cast<Computed_field*>(object_group_field)->core));
}

inline Computed_field_node_group *Computed_field_node_group_core_cast(
	cmzn_field_node_group_id object_group_field)
{
	return (static_cast<Computed_field_node_group *>(
		reinterpret_cast<Computed_field*>(object_group_field)->core));
}

/**
 * List statistics about btree structure of node_group.
 */
void cmzn_field_node_group_list_btree_statistics(
	cmzn_field_node_group_id node_group);

/**
 * List statistics about btree structure of element_group.
 */
void cmzn_field_element_group_list_btree_statistics(
	cmzn_field_element_group_id element_group);

#endif /* COMPUTED_FIELD_SUBOBJECT_GROUP_HPP */

