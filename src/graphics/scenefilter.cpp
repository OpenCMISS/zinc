/**
 * scenefilter.cpp
 *
 * Implementation of scene filter.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <list>
#include "opencmiss/zinc/scene.h"
#include "opencmiss/zinc/status.h"
#include "general/cmiss_set.hpp"
#include "general/debug.h"
#include "general/enumerator_conversion.hpp"
#include "general/indexed_list_stl_private.hpp"
#include "general/mystring.h"
#include "general/manager_private.h"
#include "graphics/element_point_ranges.h"
#include "graphics/graphics.h"
#include "graphics/graphics_module.hpp"
#include "graphics/scenefilter.hpp"
#include "region/cmiss_region.hpp"

FULL_DECLARE_MANAGER_TYPE_WITH_OWNER(cmzn_scenefilter, cmzn_scenefiltermodule, struct cmzn_scenefilter_change_detail *);

inline void MANAGER_UPDATE_DEPENDENCIES(cmzn_scenefilter)(
	struct MANAGER(cmzn_scenefilter) *manager)
{
	cmzn_set_cmzn_scenefilter *all_filters = reinterpret_cast<cmzn_set_cmzn_scenefilter *>(manager->object_list);
	for (cmzn_set_cmzn_scenefilter::iterator iter = all_filters->begin(); iter != all_filters->end(); iter++)
	{
		cmzn_scenefilter_id filter = *iter;
		filter->check_dependency();
	}
}

inline struct cmzn_scenefilter_change_detail *MANAGER_EXTRACT_CHANGE_DETAIL(cmzn_scenefilter)(
	cmzn_scenefilter *filter)
{
	return filter->extract_change_detail();
}

inline void MANAGER_CLEANUP_CHANGE_DETAIL(cmzn_scenefilter)(
	cmzn_scenefilter_change_detail **change_detail_address)
{
	delete *change_detail_address;
}

struct cmzn_scenefilter_operator : public cmzn_scenefilter
{
protected:

	struct Scenefilteroperand
	{
		cmzn_scenefilter *filter;
		bool isActive;

		Scenefilteroperand(cmzn_scenefilter *filter) :
			filter(cmzn_scenefilter_access(filter)),
			isActive(true)
		{
		}

		~Scenefilteroperand()
		{
			cmzn_scenefilter_destroy(&filter);
		}
	};

	typedef std::list<Scenefilteroperand*> OperandList;

	OperandList operands;

	OperandList::iterator find_operand(cmzn_scenefilter *operand)
	{
		for (OperandList::iterator pos = operands.begin(); pos != operands.end(); ++pos)
		{
			if ((*pos)->filter == operand)
				return pos;
		}
		return operands.end();
	}

public:
	cmzn_scenefilter_operator()
	{
		filter_type = CMZN_SCENEFILTER_TYPE_OPERATOR;
	}

	virtual ~cmzn_scenefilter_operator()
	{
		for (OperandList::iterator pos = operands.begin(); pos != operands.end(); ++pos)
		{
			delete *pos;
		}
	}

	virtual int check_dependency()
	{
		if (manager_change_status & MANAGER_CHANGE_RESULT(cmzn_scenefilter))
		{
			return 1;
		}
		for (OperandList::const_iterator pos = operands.begin(); pos != operands.end(); ++pos)
		{
			if ((*pos)->filter->check_dependency())
			{
				changed(MANAGER_CHANGE_RESULT(cmzn_scenefilter));
				return 1;
			}
		}
		return 0;
	}

	virtual bool match(struct cmzn_graphics *graphics) = 0;

	void list_operands() const
	{
		if (0 < operands.size())
		{
			display_message(INFORMATION_MESSAGE, " add_filters");
			for (OperandList::const_iterator pos = operands.begin(); pos != operands.end(); ++pos)
			{
				display_message(INFORMATION_MESSAGE, " %s", (*pos)->filter->name);
			}
		}
	}

	/**
	 * Check for circular dependencies / infinite loops.
	 * @return  true if this filter depends on other_filter, false if not.
	 */
	virtual bool depends_on_filter(const cmzn_scenefilter *other_filter) const
	{
		if (other_filter == this)
			return true;
		for (OperandList::const_iterator pos = operands.begin(); pos != operands.end(); ++pos)
		{
			if ((*pos)->filter->depends_on_filter(other_filter))
				return true;
		}
		return false;
	}

	int append_operand(cmzn_scenefilter *operand)
	{
		int return_code = 1;
		OperandList::iterator pos = find_operand(operand);
		if (pos != operands.end())
		{
			// move existing operand to end of list
			operands.push_back(*pos);
			operands.erase(pos);
		}
		else
		{
			// prevent circular dependency / infinite loop
			if (!this->depends_on_filter(operand))
			{
				operands.push_back(new Scenefilteroperand(operand));
				changed(MANAGER_CHANGE_RESULT(cmzn_scenefilter));
			}
			else
			{
				return_code = 0;
			}
		}
		return return_code;
	}

	int remove_operand(cmzn_scenefilter *operand)
	{
		int return_code = 1;
		OperandList::iterator pos = find_operand(operand);
		if (pos != operands.end())
		{
			delete *pos;
			operands.erase(pos);
			changed(MANAGER_CHANGE_RESULT(cmzn_scenefilter));
		}
		else
		{
			return_code = 0;
		}
		return return_code;
	}

	cmzn_scenefilter_id getFirstOperand()
	{
		cmzn_scenefilter_id operand = NULL;
		OperandList::iterator pos = operands.begin();
		if (pos != operands.end())
		{
			operand = (*pos)->filter->access();
		}
		return operand;
	}

	cmzn_scenefilter_id getNextOperand(cmzn_scenefilter_id ref_operand)
	{
		cmzn_scenefilter_id operand = NULL;
		OperandList::iterator pos = find_operand(ref_operand);
		pos++;
		if (pos != operands.end())
		{
			operand = (*pos)->filter->access();
		}
		return operand;
	}

	bool getOperandIsActive(cmzn_scenefilter_id operand)
	{
		OperandList::iterator pos = find_operand(operand);
		if (pos != operands.end())
			return (*pos)->isActive;
		return false;
	}

	int setOperandIsActive(cmzn_scenefilter_id operand, bool is_active)
	{
		OperandList::iterator pos = find_operand(operand);
		if (pos != operands.end())
		{
			if ((*pos)->isActive != is_active)
			{
				(*pos)->isActive = is_active;
				changed(MANAGER_CHANGE_RESULT(cmzn_scenefilter));
			}
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	int insertOperandBefore(cmzn_scenefilter_id operand, cmzn_scenefilter_id ref_operand)
	{
		int return_code = 0;
		OperandList::iterator refpos = find_operand(ref_operand);
		if (refpos != operands.end())
		{
			OperandList::iterator pos = find_operand(operand);
			if (pos != operands.end())
			{
				// move existing operand to before refpos
				operands.insert(refpos, (*pos));
				operands.erase(pos);
			}
			else
			{
				operands.insert(refpos, new Scenefilteroperand(operand));
				changed(MANAGER_CHANGE_RESULT(cmzn_scenefilter));
			}
			return_code = 1;
		}
		return return_code;
	}
};

class cmzn_scenefilter_operator_and : public cmzn_scenefilter_operator
{
public:
	cmzn_scenefilter_operator_and()
	{
		filter_type = CMZN_SCENEFILTER_TYPE_OPERATOR_AND;
	}

	virtual bool match(struct cmzn_graphics *graphics)
	{
		int return_code = 1;
		for (OperandList::iterator pos = operands.begin(); pos != operands.end(); ++pos)
		{
			if ((*pos)->isActive && (!(*pos)->filter->match(graphics)))
			{
				return_code = 0;
				break;
			}
		}
		return (!isInverse() == return_code);
	}

	virtual void list_type_specific() const
	{
		display_message(INFORMATION_MESSAGE, "operator_and");
		list_operands();
	}
};

class cmzn_scenefilter_operator_or : public cmzn_scenefilter_operator
{
public:
	cmzn_scenefilter_operator_or()
	{
		filter_type = CMZN_SCENEFILTER_TYPE_OPERATOR_OR;
	}

	virtual bool match(struct cmzn_graphics *graphics)
	{
		int return_code = 1;
		if (operands.size() > 0)
		{
			return_code = 0;
		}
		for (OperandList::iterator pos = operands.begin(); pos != operands.end(); ++pos)
		{
			if ((*pos)->isActive && (*pos)->filter->match(graphics))
			{
				return_code = 1;
				break;
			}
		}
		return (!isInverse() == return_code);
	}

	virtual void list_type_specific() const
	{
		display_message(INFORMATION_MESSAGE, "operator_or");
		list_operands();
	}
};


namespace {

class cmzn_scenefilter_graphics_name : public cmzn_scenefilter
{
	const char *matchName;

public:
	cmzn_scenefilter_graphics_name(const char *inMatchName) :
		matchName(duplicate_string(inMatchName))
	{
		filter_type = CMZN_SCENEFILTER_TYPE_GRAPHICS_NAME;
	}

	virtual ~cmzn_scenefilter_graphics_name()
	{
		DEALLOCATE(matchName);
	}

	virtual bool match(struct cmzn_graphics *graphics)
	{
		return (::cmzn_graphics_has_name(graphics, (void *)matchName) == !isInverse());
	}

	virtual void list_type_specific() const
	{
		display_message(INFORMATION_MESSAGE, "match_graphics_name %s", matchName);
	}

};

class cmzn_scenefilter_visibility_flags : public cmzn_scenefilter
{
public:
	cmzn_scenefilter_visibility_flags()
	{
		filter_type = CMZN_SCENEFILTER_TYPE_VISIBILITY_FLAGS;
	}

	virtual bool match(struct cmzn_graphics *graphics)
	{
		return (!isInverse() == cmzn_graphics_and_scene_visibility_flags_is_set(graphics));
	}

	virtual void list_type_specific() const
	{
		display_message(INFORMATION_MESSAGE, "match_visibility_flags");
	}
};

class cmzn_scenefilter_region : public cmzn_scenefilter
{
	cmzn_region *matchRegion;

public:
	cmzn_scenefilter_region(cmzn_region *inRegion) :
		matchRegion(inRegion)
	{
		filter_type = CMZN_SCENEFILTER_TYPE_REGION;
	}

	virtual bool match(struct cmzn_graphics *graphics)
	{
		return (!isInverse() == cmzn_graphics_is_from_region_hierarchical(graphics, matchRegion));
	}

	virtual void list_type_specific() const
	{
		char *region_name = cmzn_region_get_path(matchRegion);
		display_message(INFORMATION_MESSAGE, "match_region_path %s", region_name);
		DEALLOCATE(region_name);
	}
};

class cmzn_scenefilter_domain_type : public cmzn_scenefilter
{
	enum cmzn_field_domain_type domain_type;

public:
	cmzn_scenefilter_domain_type(enum cmzn_field_domain_type inDomainType) :
		domain_type(inDomainType)
	{
		filter_type = CMZN_SCENEFILTER_TYPE_DOMAIN_TYPE;
	}

	virtual bool match(struct cmzn_graphics *graphics)
	{
		return (!isInverse() == (domain_type == cmzn_graphics_get_field_domain_type(graphics)));
	}

	virtual void list_type_specific() const
	{
		display_message(INFORMATION_MESSAGE, "%s", ENUMERATOR_STRING(cmzn_field_domain_type)(domain_type));
	}
};

class cmzn_scenefilter_graphics_type : public cmzn_scenefilter
{
	enum cmzn_graphics_type graphics_type;

public:
	cmzn_scenefilter_graphics_type(enum cmzn_graphics_type inGraphicType) :
		graphics_type(inGraphicType)
	{
		filter_type = CMZN_SCENEFILTER_TYPE_GRAPHICS_TYPE;
	}

	virtual bool match(struct cmzn_graphics *graphics)
	{
		return (!isInverse() == (graphics_type == cmzn_graphics_get_type(graphics)));
	}

	virtual void list_type_specific() const
	{
		char *filter_type_name = cmzn_graphics_type_enum_to_string(graphics_type);
		display_message(INFORMATION_MESSAGE, "%s", filter_type_name);
		DEALLOCATE(filter_type_name);
	}
};
}

cmzn_scenefilter *cmzn_scenefilter_access(cmzn_scenefilter *filter)
{
	if (filter)
		filter->access();
	return filter;
}

int cmzn_scenefilter_destroy(cmzn_scenefilter **filter_address)
{
	return DEACCESS(cmzn_scenefilter)(filter_address);
}

namespace {

/***************************************************************************//**
 * Frees the memory for the scenefilter of <*scenefilter_address>.
 * Sets *scenefilter_address to NULL.
 */
int DESTROY(cmzn_scenefilter)(struct cmzn_scenefilter **scenefilter_address)
{
	int return_code;

	ENTER(DESTROY(cmzn_scenefilter));
	if (scenefilter_address && (*scenefilter_address))
	{
		delete *scenefilter_address;
		*scenefilter_address = NULL;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROY(cmzn_scenefilter).  Invalid argument");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

DECLARE_MANAGER_UPDATE_FUNCTION(cmzn_scenefilter)

DECLARE_MANAGED_OBJECT_NOT_IN_USE_CONDITIONAL_FUNCTION(cmzn_scenefilter)

} /* anonymous namespace */

DECLARE_ACCESS_OBJECT_FUNCTION(cmzn_scenefilter)

/***************************************************************************//**
 * Custom version handling is_managed_flag.
 */
PROTOTYPE_DEACCESS_OBJECT_FUNCTION(cmzn_scenefilter)
{
	int return_code;
	cmzn_scenefilter *object;

	ENTER(DEACCESS(cmzn_scenefilter));
	if (object_address && (object = *object_address))
	{
		(object->access_count)--;
		if (object->access_count <= 0)
		{
			delete object;
			return_code = 1;
		}
		else if ((!object->is_managed_flag) && (object->manager) &&
			((1 == object->access_count) || ((2 == object->access_count) &&
				(MANAGER_CHANGE_NONE(cmzn_scenefilter) != object->manager_change_status))))
		{
			return_code =
				REMOVE_OBJECT_FROM_MANAGER(cmzn_scenefilter)(object, object->manager);
		}
		else
		{
			return_code = 1;
		}
		*object_address = (struct cmzn_scenefilter *)NULL;
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* DEACCESS(cmzn_scenefilter) */

PROTOTYPE_REACCESS_OBJECT_FUNCTION(cmzn_scenefilter)
{
	int return_code;

	ENTER(REACCESS(cmzn_scenefilter));
	if (object_address)
	{
		return_code = 1;
		if (new_object)
		{
			/* access the new object */
			(new_object->access_count)++;
		}
		if (*object_address)
		{
			/* deaccess the current object */
			DEACCESS(cmzn_scenefilter)(object_address);
		}
		/* point to the new object */
		*object_address = new_object;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"REACCESS(cmzn_scenefilter).  Invalid argument");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* REACCESS(cmzn_scenefilter) */

DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(cmzn_scenefilter)

DECLARE_INDEXED_LIST_STL_FUNCTIONS(cmzn_scenefilter)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL_FUNCTION(cmzn_scenefilter,name,const char *)

DECLARE_MANAGER_FUNCTIONS(cmzn_scenefilter,manager)
DECLARE_DEFAULT_MANAGED_OBJECT_NOT_IN_USE_FUNCTION(cmzn_scenefilter,manager)
DECLARE_MANAGER_IDENTIFIER_WITHOUT_MODIFY_FUNCTIONS(cmzn_scenefilter,name,const char *,manager)
DECLARE_MANAGER_OWNER_FUNCTIONS(cmzn_scenefilter, struct cmzn_scenefiltermodule)

int cmzn_scenefilter_manager_set_owner_private(struct MANAGER(cmzn_scenefilter) *manager,
	struct cmzn_scenefiltermodule *scenefiltermodule)
{
	return MANAGER_SET_OWNER(cmzn_scenefilter)(manager, scenefiltermodule);
}


struct cmzn_scenefiltermodule
{

private:

	struct MANAGER(cmzn_scenefilter) *scenefilterManager;
	cmzn_scenefilter *defaultScenefilter;
	int access_count;

	cmzn_scenefiltermodule() :
		scenefilterManager(CREATE(MANAGER(cmzn_scenefilter))()),
		defaultScenefilter(0),
		access_count(1)
	{
	}

	~cmzn_scenefiltermodule()
	{
		if (defaultScenefilter)
		{
			DEACCESS(cmzn_scenefilter)(&(this->defaultScenefilter));
		}
		DESTROY(MANAGER(cmzn_scenefilter))(&(this->scenefilterManager));
	}

public:

	static cmzn_scenefiltermodule *create()
	{
		cmzn_scenefiltermodule *filtermodule = new cmzn_scenefiltermodule();
		cmzn_scenefilter_id filter = filtermodule->getDefaultFilter();
		cmzn_scenefilter_destroy(&filter);
		return filtermodule;
	}

	cmzn_scenefiltermodule *access()

	{
		++access_count;
		return this;
	}

	static int deaccess(cmzn_scenefiltermodule* &scenefiltermodule)
	{
		if (scenefiltermodule)
		{
			--(scenefiltermodule->access_count);
			if (scenefiltermodule->access_count <= 0)
			{
				delete scenefiltermodule;
			}
			scenefiltermodule = 0;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	struct MANAGER(cmzn_scenefilter) *getManager()
	{
		return this->scenefilterManager;
	}

	int beginChange()
	{
		return MANAGER_BEGIN_CACHE(cmzn_scenefilter)(this->scenefilterManager);
	}

	int endChange()
	{
		return MANAGER_END_CACHE(cmzn_scenefilter)(this->scenefilterManager);
	}

	char *getValidTemporaryNameForScenefilter()
	{
		char *name = NULL;
		if (scenefilterManager)
		{
			char temp_name[20];
			int i = NUMBER_IN_MANAGER(cmzn_scenefilter)(scenefilterManager);
			do
			{
				i++;
				sprintf(temp_name, "temp%d",i);
			}
			while (FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_scenefilter,name)(temp_name,
				scenefilterManager));
			name = duplicate_string(temp_name);
		}
		return name;
	}

	cmzn_scenefilter *createFilterVisibilityFlags()
	{
		cmzn_scenefilter_id scenefilter = NULL;
		if (scenefilterManager)
		{
			char *name = getValidTemporaryNameForScenefilter();
			scenefilter = new cmzn_scenefilter_visibility_flags();
			cmzn_scenefilter_set_name(scenefilter, name);
			if (!ADD_OBJECT_TO_MANAGER(cmzn_scenefilter)(scenefilter, scenefilterManager))
			{
				DEACCESS(cmzn_scenefilter)(&scenefilter);
			}
			DEALLOCATE(name);
		}
		return scenefilter;
	}

	cmzn_scenefilter_id createFilterDomainType(
		enum cmzn_field_domain_type domain_type)
	{
		cmzn_scenefilter_id scenefilter = 0;
		if (scenefilterManager)
		{
			scenefilter = new cmzn_scenefilter_domain_type(domain_type);
			char *name = getValidTemporaryNameForScenefilter();
			cmzn_scenefilter_set_name(scenefilter, name);
			DEALLOCATE(name);
			if (!ADD_OBJECT_TO_MANAGER(cmzn_scenefilter)(scenefilter, scenefilterManager))
			{
				DEACCESS(cmzn_scenefilter)(&scenefilter);
			}
		}
		return scenefilter;
	}

	cmzn_scenefilter *createFilterGraphicName(const char *match_name)
	{
		cmzn_scenefilter_id scenefilter = NULL;
		if (scenefilterManager && match_name)
		{
			char *name = getValidTemporaryNameForScenefilter();
			scenefilter = new cmzn_scenefilter_graphics_name(match_name);
			cmzn_scenefilter_set_name(scenefilter, name);
			if (!ADD_OBJECT_TO_MANAGER(cmzn_scenefilter)(scenefilter, scenefilterManager))
			{
				DEACCESS(cmzn_scenefilter)(&scenefilter);
			}
			DEALLOCATE(name);
		}
		return scenefilter;
	}

	cmzn_scenefilter_id createFilterGraphicType(
		enum cmzn_graphics_type graphics_type)
	{
		cmzn_scenefilter_id scenefilter = NULL;
		if (scenefilterManager)
		{
			char *name = getValidTemporaryNameForScenefilter();
			scenefilter = new cmzn_scenefilter_graphics_type(graphics_type);
			cmzn_scenefilter_set_name(scenefilter, name);
			if (!ADD_OBJECT_TO_MANAGER(cmzn_scenefilter)(scenefilter, scenefilterManager))
			{
				DEACCESS(cmzn_scenefilter)(&scenefilter);
			}
			DEALLOCATE(name);
		}
		return scenefilter;
	}

	cmzn_scenefilter_id createFilterRegion(
		cmzn_region_id match_region)
	{
		cmzn_scenefilter_id scenefilter = NULL;
		if (scenefilterManager && match_region)
		{
			char *name = getValidTemporaryNameForScenefilter();
			scenefilter =  new cmzn_scenefilter_region(match_region);
			cmzn_scenefilter_set_name(scenefilter, name);
			if (!ADD_OBJECT_TO_MANAGER(cmzn_scenefilter)(scenefilter, scenefilterManager))
			{
				DEACCESS(cmzn_scenefilter)(&scenefilter);
			}
			DEALLOCATE(name);
		}
		return scenefilter;
	}

	cmzn_scenefilter_id createFilterOperatorAnd()
	{
		cmzn_scenefilter_id scenefilter = NULL;
		if (scenefilterManager)
		{
			char *name = getValidTemporaryNameForScenefilter();
			scenefilter = new cmzn_scenefilter_operator_and();
			cmzn_scenefilter_set_name(scenefilter, name);
			if (!ADD_OBJECT_TO_MANAGER(cmzn_scenefilter)(scenefilter, scenefilterManager))
			{
				DEACCESS(cmzn_scenefilter)(&scenefilter);
			}
			DEALLOCATE(name);
		}
		return scenefilter;
	}

	cmzn_scenefilter_id createFilterOperatorOr()
	{
		cmzn_scenefilter_id scenefilter = NULL;
		if (scenefilterManager)
		{
			char *name = getValidTemporaryNameForScenefilter();
			scenefilter = new cmzn_scenefilter_operator_or();
			cmzn_scenefilter_set_name(scenefilter, name);
			if (!ADD_OBJECT_TO_MANAGER(cmzn_scenefilter)(scenefilter, scenefilterManager))
			{
				DEACCESS(cmzn_scenefilter)(&scenefilter);
			}
			DEALLOCATE(name);
		}
		return scenefilter;
	}

	cmzn_scenefilter *findFilterByName(const char *name)
	{
		cmzn_scenefilter *scenefilter = FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_scenefilter,name)(name,
			this->scenefilterManager);
		if (scenefilter)
		{
			return ACCESS(cmzn_scenefilter)(scenefilter);
		}
		return 0;
	}

	cmzn_scenefilter *getDefaultFilter()
	{
		if (this->defaultScenefilter)
		{
			ACCESS(cmzn_scenefilter)(this->defaultScenefilter);
			return this->defaultScenefilter;
		}
		else
		{
			const char *default_scenefilter_name = "default";
			struct cmzn_scenefilter *scenefilter = findFilterByName(default_scenefilter_name);
			if (NULL == scenefilter)
			{
				scenefilter = createFilterVisibilityFlags();
				cmzn_scenefilter_set_name(scenefilter, default_scenefilter_name);
				cmzn_scenefilter_set_managed(scenefilter, true);
			}
			if (scenefilter)
			{
				setDefaultFilter(scenefilter);
				cmzn_scenefilter_set_managed(scenefilter, true);
			}
			return scenefilter;
		}

		return 0;
	}

	int setDefaultFilter(cmzn_scenefilter *scenefilter)
	{
		REACCESS(cmzn_scenefilter)(&this->defaultScenefilter, scenefilter);
		return CMZN_OK;
	}

};

cmzn_scenefilter_id cmzn_scenefiltermodule_create_scenefilter_visibility_flags(
	cmzn_scenefiltermodule_id scenefiltermodule)
{
	if (scenefiltermodule)
	{
		return scenefiltermodule->createFilterVisibilityFlags();
	}
	return 0;
}

cmzn_scenefilter_id cmzn_scenefiltermodule_create_scenefilter_field_domain_type(
	cmzn_scenefiltermodule_id scenefiltermodule,
	enum cmzn_field_domain_type domain_type)
{
	if (scenefiltermodule)
	{
		return scenefiltermodule->createFilterDomainType(domain_type);
	}
	return 0;

}

cmzn_scenefilter_id cmzn_scenefiltermodule_create_scenefilter_graphics_name(
	cmzn_scenefiltermodule_id scenefiltermodule, const char *match_name)
{
	if (scenefiltermodule)
	{
		return scenefiltermodule->createFilterGraphicName(match_name);
	}
	return 0;
}

cmzn_scenefilter_id cmzn_scenefiltermodule_create_scenefilter_graphics_type(
	cmzn_scenefiltermodule_id scenefiltermodule, enum cmzn_graphics_type graphics_type)
{
	if (scenefiltermodule)
	{
		return scenefiltermodule->createFilterGraphicType(graphics_type);
	}
	return 0;
}

cmzn_scenefilter_id cmzn_scenefiltermodule_create_scenefilter_region(
	cmzn_scenefiltermodule_id scenefiltermodule, cmzn_region_id match_region)
{
	if (scenefiltermodule)
	{
		return scenefiltermodule->createFilterRegion(match_region);
	}
	return 0;
}

cmzn_scenefilter_id cmzn_scenefiltermodule_create_scenefilter_operator_and(
	cmzn_scenefiltermodule_id scenefiltermodule)
{
	if (scenefiltermodule)
	{
		return scenefiltermodule->createFilterOperatorAnd();
	}
	return 0;
}

cmzn_scenefilter_id cmzn_scenefiltermodule_create_scenefilter_operator_or(
	cmzn_scenefiltermodule_id scenefiltermodule)
{
	if (scenefiltermodule)
	{
		return scenefiltermodule->createFilterOperatorOr();
	}
	return 0;
}

cmzn_scenefiltermodule_id cmzn_scenefiltermodule_create()
{
	return cmzn_scenefiltermodule::create();
}

cmzn_scenefiltermodule_id cmzn_scenefiltermodule_access(
	cmzn_scenefiltermodule_id scenefiltermodule)
{
	if (scenefiltermodule)
		return scenefiltermodule->access();
	return 0;
}

int cmzn_scenefiltermodule_destroy(cmzn_scenefiltermodule_id *scenefiltermodule_address)
{
	if (scenefiltermodule_address)
		return cmzn_scenefiltermodule::deaccess(*scenefiltermodule_address);
	return CMZN_ERROR_ARGUMENT;
}

struct MANAGER(cmzn_scenefilter) *cmzn_scenefiltermodule_get_manager(
	cmzn_scenefiltermodule_id scenefiltermodule)
{
	if (scenefiltermodule)
		return scenefiltermodule->getManager();
	return 0;
}

int cmzn_scenefiltermodule_begin_change(cmzn_scenefiltermodule_id scenefiltermodule)
{
	if (scenefiltermodule)
		return scenefiltermodule->beginChange();
   return CMZN_ERROR_ARGUMENT;
}

int cmzn_scenefiltermodule_end_change(cmzn_scenefiltermodule_id scenefiltermodule)
{
	if (scenefiltermodule)
		return scenefiltermodule->endChange();
   return CMZN_ERROR_ARGUMENT;
}

cmzn_scenefilter_id cmzn_scenefiltermodule_find_scenefilter_by_name(
	cmzn_scenefiltermodule_id scenefiltermodule, const char *name)
{
	if (scenefiltermodule)
		return scenefiltermodule->findFilterByName(name);
   return 0;
}

cmzn_scenefilter_id cmzn_scenefiltermodule_get_default_scenefilter(
	cmzn_scenefiltermodule_id scenefiltermodule)
{
	if (scenefiltermodule)
		return scenefiltermodule->getDefaultFilter();
	return 0;
}

int cmzn_scenefiltermodule_set_default_scenefilter(
	cmzn_scenefiltermodule_id scenefiltermodule,
	cmzn_scenefilter_id scenefilter)
{
	if (scenefiltermodule)
		return scenefiltermodule->setDefaultFilter(scenefilter);
	return 0;
}

char *cmzn_scenefilter_get_name(cmzn_scenefilter_id scenefilter)
{
	char *name = NULL;

	if (scenefilter)
	{
		name = scenefilter->getName();
	}
	return name;
}

int cmzn_scenefilter_set_name(cmzn_scenefilter_id scenefilter, const char *name)
{
	int return_code;

	ENTER(cmzn_scenefilter_set_name);
	if (scenefilter && name)
	{
		return_code = 1;
		cmzn_set_cmzn_scenefilter *manager_scenefilter_list = 0;
		bool restore_changed_object_to_lists = false;
		if (scenefilter->manager)
		{
			if (FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_scenefilter, name)(
				name, scenefilter->manager))
			{
				display_message(ERROR_MESSAGE, "cmzn_scenefilter_set_name.  "
					"scene filter named '%s' already exists.", name);
				return_code = 0;
			}
			if (return_code)
			{
				manager_scenefilter_list = reinterpret_cast<cmzn_set_cmzn_scenefilter *>(
					scenefilter->manager->object_list);
				// this temporarily removes the object from all related lists
				restore_changed_object_to_lists =
					manager_scenefilter_list->begin_identifier_change(scenefilter);
				if (!restore_changed_object_to_lists)
				{
					display_message(ERROR_MESSAGE, "cmzn_scenefilter_set_name.  "
						"Could not safely change identifier in manager");
					return_code = 0;
				}
			}
		}
		if (return_code)
		{
			if (name)
			{
				scenefilter->setName(name);
			}
			else
			{
				return_code = 0;
			}
		}
		if (restore_changed_object_to_lists)
		{
			manager_scenefilter_list->end_identifier_change();
		}
		if (scenefilter->manager && return_code)
		{
			MANAGED_OBJECT_CHANGE(cmzn_scenefilter)(scenefilter,
				MANAGER_CHANGE_IDENTIFIER(cmzn_scenefilter));
		}
	}
	else
	{
		if (scenefilter)
		{
			display_message(ERROR_MESSAGE,
				"cmzn_scenefilter_set_name.  Invalid scene filter name '%s'", name);
		}
		return_code=0;
	}

	return (return_code);
}

bool cmzn_scenefilter_evaluate_graphics(cmzn_scenefilter_id filter,
	cmzn_graphics_id graphics)
{
	if (filter && graphics)
		return filter->match(graphics);
	return false;
}

bool cmzn_scenefilter_is_managed(cmzn_scenefilter_id filter)
{
	if (filter)
	{
		return filter->is_managed_flag;
	}
	return 0;
}

int cmzn_scenefilter_set_managed(cmzn_scenefilter_id filter,
	bool value)
{
	if (filter)
	{
		bool old_value = filter->is_managed_flag;
		filter->is_managed_flag = (value != 0);
		if (value != old_value)
		{
			MANAGED_OBJECT_CHANGE(cmzn_scenefilter)(filter,
				MANAGER_CHANGE_DEFINITION(cmzn_scenefilter));
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_scenefilter_is_inverse(cmzn_scenefilter_id filter)
{
	if (filter)
		return filter->isInverse();
	return false;
}

int cmzn_scenefilter_set_inverse(cmzn_scenefilter_id filter,
	bool value)
{
	if (filter)
	{
		filter->setInverse(value);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_scenefilter_operator_id cmzn_scenefilter_cast_operator(cmzn_scenefilter_id scenefilter)
{
	if (dynamic_cast<cmzn_scenefilter_operator*>(scenefilter))
	{
		cmzn_scenefilter_access(scenefilter);
		return (reinterpret_cast<cmzn_scenefilter_operator_id>(scenefilter));
	}
	else
	{
		return (NULL);
	}
}

int cmzn_scenefilter_operator_destroy(cmzn_scenefilter_operator_id *operator_filter_address)
{
	return cmzn_scenefilter_destroy(reinterpret_cast<cmzn_scenefilter_id *>(operator_filter_address));
}

int cmzn_scenefilter_operator_append_operand(
	cmzn_scenefilter_operator_id operator_filter,
	cmzn_scenefilter_id operand)
{
	if (operator_filter && operand)
	{
		return operator_filter->append_operand(operand);
	}
	return 0;
}

cmzn_scenefilter_id cmzn_scenefilter_operator_get_first_operand(
	cmzn_scenefilter_operator_id operator_filter)
{
	cmzn_scenefilter_id operand = NULL;
	if (operator_filter)
	{
		operand = operator_filter->getFirstOperand();
	}
	return operand;
}

cmzn_scenefilter_id cmzn_scenefilter_operator_get_next_operand(
	cmzn_scenefilter_operator_id operator_filter,
	cmzn_scenefilter_id ref_operand)
{
	cmzn_scenefilter_id operand = NULL;
	if (operator_filter && ref_operand)
	{
		operand = operator_filter->getNextOperand(ref_operand);
	}
	return operand;
}

bool cmzn_scenefilter_operator_is_operand_active(
	cmzn_scenefilter_operator_id operator_filter,
	cmzn_scenefilter_id operand)
{
	if (operator_filter && operand)
		return operator_filter->getOperandIsActive(operand);
	return false;
}

int cmzn_scenefilter_operator_set_operand_active(
	cmzn_scenefilter_operator_id operator_filter,
	cmzn_scenefilter_id operand, bool is_active)
{
	if (operator_filter && operand)
		return operator_filter->setOperandIsActive(operand, is_active);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_scenefilter_operator_insert_operand_before(
	cmzn_scenefilter_operator_id operator_filter,
	cmzn_scenefilter_id operand, cmzn_scenefilter_id ref_operand)
{
	int return_code = 0;
	if (operator_filter && operand && ref_operand)
	{
		return_code = operator_filter->insertOperandBefore(operand, ref_operand);
	}
	return return_code;
}

int cmzn_scenefilter_operator_remove_operand(
	cmzn_scenefilter_operator_id operator_filter,
	cmzn_scenefilter_id operand)
{
	if (operator_filter && operand)
	{
		return operator_filter->remove_operand(operand);
	}
	return 0;
}
