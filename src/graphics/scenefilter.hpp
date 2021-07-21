/**
 * scenefilter.hpp
 *
 * Declaration of scene filter classes and functions.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SCENEFILTER_PRIVATE_HPP
#define SCENEFILTER_PRIVATE_HPP

#include "general/list.h"
#include "general/manager.h"
#include "general/mystring.h"
#include "general/object.h"
#include "general/debug.h"
#include "general/cmiss_set.hpp"

struct cmzn_scene;
struct cmzn_graphics;

enum cmzn_scenefilter_type
{
	CMZN_SCENEFILTER_TYPE_INVALID,
	CMZN_SCENEFILTER_TYPE_BASE,
	CMZN_SCENEFILTER_TYPE_IDENTIFIER ,
	CMZN_SCENEFILTER_TYPE_DOMAIN_TYPE,
	CMZN_SCENEFILTER_TYPE_GRAPHICS_NAME,
	CMZN_SCENEFILTER_TYPE_GRAPHICS_TYPE,
	CMZN_SCENEFILTER_TYPE_VISIBILITY_FLAGS ,
	CMZN_SCENEFILTER_TYPE_REGION,
	CMZN_SCENEFILTER_TYPE_OPERATOR,
	CMZN_SCENEFILTER_TYPE_OPERATOR_AND,
	CMZN_SCENEFILTER_TYPE_OPERATOR_OR
};

DECLARE_LIST_TYPES(cmzn_scenefilter);

DECLARE_MANAGER_TYPES(cmzn_scenefilter);

PROTOTYPE_OBJECT_FUNCTIONS(cmzn_scenefilter);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(cmzn_scenefilter);

PROTOTYPE_LIST_FUNCTIONS(cmzn_scenefilter);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(cmzn_scenefilter,name,const char *);

PROTOTYPE_MANAGER_FUNCTIONS(cmzn_scenefilter);
PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(cmzn_scenefilter,name,const char *);

struct cmzn_scenefilter_change_detail
{
	virtual ~cmzn_scenefilter_change_detail()
	{
	}
};

struct cmzn_scenefilter
{
private:
	bool inverse;

public:
	enum cmzn_scenefilter_type filter_type;
	const char *name;
	int access_count;
	struct MANAGER(cmzn_scenefilter) *manager;
	int manager_change_status;
	bool is_managed_flag;


	cmzn_scenefilter() :
		inverse(false),
		filter_type(CMZN_SCENEFILTER_TYPE_BASE),
		name(NULL),
		access_count(1),
		manager(NULL),
		manager_change_status(MANAGER_CHANGE_NONE(cmzn_scenefilter)),
		is_managed_flag(false)
	{
	}

	virtual ~cmzn_scenefilter()
	{
		DEALLOCATE(name);
	}

	virtual bool match(struct cmzn_graphics *graphics) = 0;

	bool setName(const char *name_in)
	{
		char *new_name = duplicate_string(name_in);
		if (!new_name)
			return false;
		if (name)
			DEALLOCATE(name);
		name = new_name;
		return true;
	}

	char *getName()
	{
		char *name_out = NULL;
		if (name)
		{
			name_out = duplicate_string(name);
		}

		return name_out;
	}

	int changed(enum MANAGER_CHANGE(cmzn_scenefilter) change)
	{
		return MANAGED_OBJECT_CHANGE(cmzn_scenefilter)(this,
			change);
	}

	bool isInverse() const
	{
		return inverse;
	}

	bool setInverse(bool newInverse)
	{
		if (newInverse != inverse)
		{
			inverse = newInverse;
			changed(MANAGER_CHANGE_RESULT(cmzn_scenefilter));
		}
		return true;
	}

	inline cmzn_scenefilter *access()
	{
		++access_count;
		return this;
	}

	enum cmzn_scenefilter_type getType()
	{
		return filter_type;
	};

	void list(const char *prefix) const
	{
		display_message(INFORMATION_MESSAGE, "%s %s %s", prefix, name, inverse ? "inverse_match " : "normal_match ");
		list_type_specific();
		display_message(INFORMATION_MESSAGE, ";\n");
	}

	virtual void list_type_specific() const = 0;

	virtual int check_dependency()
	{
		if (manager_change_status & MANAGER_CHANGE_RESULT(cmzn_scenefilter))
		{
			return 1;
		}
		return 0;
	}

	/** clones and clears type-specific change detail, if any.
	 * override for classes with type-specific change detail
	 * @return  change detail prior to clearing, or NULL if none.
	 */
	virtual cmzn_scenefilter_change_detail *extract_change_detail()
	{
		return NULL;
	}

	/**
	 * override for filter types with that are functions of other filters to
	 * prevent circular dependencies / infinite loops.
	 * @return  true if this filter depends on other_filter, false if not.
	 */
	virtual bool depends_on_filter(const cmzn_scenefilter *other_filter) const
	{
		return (other_filter == this);
	}

	static inline int deaccess(cmzn_scenefilter **scenefilter_address)
	{
		return DEACCESS(cmzn_scenefilter)(scenefilter_address);
	}
};

/* Only to be used from FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL function
 * Creates a pseudo object with name identifier suitable for finding
 * objects by identifier with cmzn_set.
 */
class cmzn_scenefilter_identifier : private cmzn_scenefilter
{
public:
	cmzn_scenefilter_identifier(const char *name)
	{
		// const_cast OK as must never be modified & cleared in destructor
		cmzn_scenefilter::name = name;
		filter_type = CMZN_SCENEFILTER_TYPE_IDENTIFIER;
	}

	virtual ~cmzn_scenefilter_identifier()
	{
		cmzn_scenefilter::name = NULL;
	}

	virtual bool match(struct cmzn_graphics *graphics)
	{
		USE_PARAMETER(graphics);
		return false;
	}

	virtual void list_type_specific() const
	{
	}

	cmzn_scenefilter *getPseudoObject()
	{
		return this;
	}
};

/** functor for ordering cmzn_set<cmzn_scenefilter> by name */
struct cmzn_scenefilter_compare_name
{
	bool operator() (const cmzn_scenefilter * scenefilter1, const cmzn_scenefilter * scenefilter2) const
	{
		return strcmp(scenefilter1->name, scenefilter2->name) < 0;
	}
};

typedef cmzn_set<cmzn_scenefilter *,cmzn_scenefilter_compare_name> cmzn_set_cmzn_scenefilter;

int cmzn_scenefilter_manager_set_owner_private(struct MANAGER(cmzn_scenefilter) *manager,
	struct cmzn_scenefiltermodule *scenefiltermodule);

struct MANAGER(cmzn_scenefilter) *cmzn_scenefiltermodule_get_manager(
	struct cmzn_scenefiltermodule *scenefiltermodule);

cmzn_scenefiltermodule_id cmzn_scenefiltermodule_create();

#endif /* SCENEFILTER_PRIVATE_HPP_ */
