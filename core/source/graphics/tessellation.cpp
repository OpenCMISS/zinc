/***************************************************************************//**
 * tessellation.cpp
 *
 * Objects for describing how elements / continuous field domains are
 * tessellated or sampled into graphics.
 */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is cmgui.
 *
 * The Initial Developer of the Original Code is
 * Auckland Uniservices Ltd, Auckland, New Zealand.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include <cstdlib>
#include "zinc/status.h"
#include "general/debug.h"
#include "general/manager_private.h"
#include "general/mystring.h"
#include "general/cmiss_set.hpp"
#include "general/enumerator_conversion.hpp"
#include "general/indexed_list_stl_private.hpp"
#include "graphics/tessellation.hpp"
#include "general/message.h"

/*
Module types
------------
*/

/* forward declaration */
struct Cmiss_tessellation *Cmiss_tessellation_create_private();

struct Cmiss_tessellation_module
{

private:

	struct MANAGER(Cmiss_tessellation) *tessellationManager;
	Cmiss_tessellation *defaultTessellation;
	Cmiss_tessellation *defaultPointsTessellation;
	int access_count;

	Cmiss_tessellation_module() :
		tessellationManager(CREATE(MANAGER(Cmiss_tessellation))()),
		defaultTessellation(0),
		defaultPointsTessellation(0),
		access_count(1)
	{
	}

	~Cmiss_tessellation_module()
	{
		Cmiss_tessellation_destroy(&this->defaultTessellation);
		Cmiss_tessellation_destroy(&this->defaultPointsTessellation);
		DESTROY(MANAGER(Cmiss_tessellation))(&(this->tessellationManager));
	}

public:

	static Cmiss_tessellation_module *create()
	{
		return new Cmiss_tessellation_module();
	}

	Cmiss_tessellation_module *access()

	{
		++access_count;
		return this;
	}

	static int deaccess(Cmiss_tessellation_module* &tessellation_module)
	{
		if (tessellation_module)
		{
			--(tessellation_module->access_count);
			if (tessellation_module->access_count <= 0)
			{
				delete tessellation_module;
			}
			tessellation_module = 0;
			return CMISS_OK;
		}
		return CMISS_ERROR_ARGUMENT;
	}

	struct MANAGER(Cmiss_tessellation) *getManager()
	{
		return this->tessellationManager;
	}

	int beginChange()
	{
		return MANAGER_BEGIN_CACHE(Cmiss_tessellation)(this->tessellationManager);
	}

	int endChange()
	{
		return MANAGER_END_CACHE(Cmiss_tessellation)(this->tessellationManager);
	}

	Cmiss_tessellation_id createTessellation()
	{
		Cmiss_tessellation_id tessellation = NULL;
		char temp_name[20];
		int i = NUMBER_IN_MANAGER(Cmiss_tessellation)(this->tessellationManager);
		do
		{
			i++;
			sprintf(temp_name, "temp%d",i);
		}
		while (FIND_BY_IDENTIFIER_IN_MANAGER(Cmiss_tessellation,name)(temp_name,
			this->tessellationManager));
		tessellation = Cmiss_tessellation_create_private();
		Cmiss_tessellation_set_name(tessellation, temp_name);
		if (!ADD_OBJECT_TO_MANAGER(Cmiss_tessellation)(tessellation, this->tessellationManager))
		{
			DEACCESS(Cmiss_tessellation)(&tessellation);
		}
		return tessellation;
	}

	Cmiss_tessellation *findTessellationByName(const char *name)
	{
		Cmiss_tessellation *tessellation = FIND_BY_IDENTIFIER_IN_MANAGER(Cmiss_tessellation,name)(name,
			this->tessellationManager);
		if (tessellation)
		{
			return ACCESS(Cmiss_tessellation)(tessellation);
		}
		return 0;
	}

	Cmiss_tessellation *getDefaultTessellation()
	{
		if (this->defaultTessellation)
		{
			ACCESS(Cmiss_tessellation)(this->defaultTessellation);
		}
		else
		{
			this->beginChange();
			Cmiss_tessellation *tessellation = createTessellation();
			Cmiss_tessellation_set_name(tessellation, "default");
			const int default_minimum_divisions = 1;
			Cmiss_tessellation_set_minimum_divisions(tessellation,
				/*dimensions*/1, &default_minimum_divisions);
			const int default_refinement_factor = 4;
			Cmiss_tessellation_set_refinement_factors(tessellation,
				/*dimensions*/1, &default_refinement_factor);
			Cmiss_tessellation_set_circle_divisions(tessellation, 12);
			this->setDefaultTessellation(tessellation);
			this->endChange();
		}
		return this->defaultTessellation;
	}

	int setDefaultTessellation(Cmiss_tessellation *tessellation)
	{
		REACCESS(Cmiss_tessellation)(&this->defaultTessellation, tessellation);
		return CMISS_OK;
	}

	Cmiss_tessellation *getDefaultPointsTessellation()
	{
		if (this->defaultPointsTessellation)
		{
			ACCESS(Cmiss_tessellation)(this->defaultPointsTessellation);
		}
		else
		{
			this->beginChange();
			Cmiss_tessellation *tessellation = createTessellation();
			Cmiss_tessellation_set_name(tessellation, "default_points");
			const int default_minimum_divisions = 1;
			Cmiss_tessellation_set_minimum_divisions(tessellation,
				/*dimensions*/1, &default_minimum_divisions);
			const int default_refinement_factor = 1;
			Cmiss_tessellation_set_refinement_factors(tessellation,
				/*dimensions*/1, &default_refinement_factor);
			Cmiss_tessellation_set_circle_divisions(tessellation, 12);
			this->setDefaultPointsTessellation(tessellation);
			this->endChange();
		}
		return this->defaultPointsTessellation;
	}

	int setDefaultPointsTessellation(Cmiss_tessellation *tessellation)
	{
		REACCESS(Cmiss_tessellation)(&this->defaultPointsTessellation, tessellation);
		return CMISS_OK;
	}

};


void list_divisions(int size, int *divisions)
{
	for (int i = 0; i < size; i++)
	{
		if (i)
		{
			display_message(INFORMATION_MESSAGE, "*");
		}
		display_message(INFORMATION_MESSAGE, "%d", divisions[i]);
	}
}

/***************************************************************************//**
 * Object describing how elements / continuous field domains are tessellated
 * or sampled into graphics.
 */
struct Cmiss_tessellation
{
	const char *name;
	/* after clearing in create, following to be modified only by manager */
	struct MANAGER(Cmiss_tessellation) *manager;
	int manager_change_status;
	int circleDivisions;
	int minimum_divisions_size;
	int *minimum_divisions;
	int refinement_factors_size;
	int *refinement_factors;
	Cmiss_tessellation_change_detail changeDetail;
	bool is_managed_flag;
	int access_count;

protected:

	Cmiss_tessellation() :
		name(NULL),
		manager(NULL),
		manager_change_status(MANAGER_CHANGE_NONE(Cmiss_tessellation)),
		circleDivisions(12),
		minimum_divisions_size(1),
		minimum_divisions(NULL),
		refinement_factors_size(1),
		refinement_factors(NULL),
		is_managed_flag(false),
		access_count(1)
	{
		ALLOCATE(minimum_divisions, int, minimum_divisions_size);
		minimum_divisions[0] = 1;
		ALLOCATE(refinement_factors, int, refinement_factors_size);
		refinement_factors[0] = 1;
	}

	~Cmiss_tessellation()
	{
		if (name)
		{
			DEALLOCATE(name);
		}
		if (minimum_divisions)
		{
			DEALLOCATE(minimum_divisions);
		}
		if (refinement_factors)
		{
			DEALLOCATE(refinement_factors);
		}
	}

public:

	/** must construct on the heap with this function */
	static Cmiss_tessellation *create()
	{
		return new Cmiss_tessellation();
	}

	Cmiss_tessellation& operator=(const Cmiss_tessellation& source)
	{
		this->set_minimum_divisions(source.minimum_divisions_size, source.minimum_divisions);
		this->set_refinement_factors(source.refinement_factors_size, source.refinement_factors);
		this->setCircleDivisions(source.circleDivisions);
		return *this;
	}

	Cmiss_tessellation_change_detail *extractChangeDetail()
	{
		Cmiss_tessellation_change_detail *change_detail = new Cmiss_tessellation_change_detail(this->changeDetail);
		this->changeDetail.clear();
		return change_detail;
	}

	int getCircleDivisions() const
	{
		return this->circleDivisions;
	}

	int setCircleDivisions(int inCircleDivisions)
	{
		int useCircleDivisions = (inCircleDivisions > 3) ? inCircleDivisions : 3;
		if (useCircleDivisions != this->circleDivisions)
		{
			this->circleDivisions = useCircleDivisions;
			this->changeDetail.setCircleDivisionsChanged();
			MANAGED_OBJECT_CHANGE(Cmiss_tessellation)(this,
				MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(Cmiss_tessellation));
		}
		return (inCircleDivisions == this->circleDivisions) ? CMISS_OK : CMISS_ERROR_ARGUMENT;
	}

	/** get minimum divisions for a particular dimension >= 0 */
	inline int get_minimum_divisions_value(int dimension)
	{
		if (dimension < minimum_divisions_size)
		{
			return minimum_divisions[dimension];
		}
		else if (minimum_divisions_size)
		{
			return minimum_divisions[minimum_divisions_size - 1];
		}
		return 1;
	}

	/** get refinement_factors value for a particular dimension >= 0 */
	inline int get_refinement_factors_value(int dimension)
	{
		if (dimension < refinement_factors_size)
		{
			return refinement_factors[dimension];
		}
		else if (refinement_factors_size)
		{
			return refinement_factors[refinement_factors_size - 1];
		}
		return 1;
	}

	/** assumes arguments have been checked already */
	int set_minimum_divisions(int dimensions, const int *in_minimum_divisions)
	{
		if (dimensions > minimum_divisions_size)
		{
			int *temp;
			if (!REALLOCATE(temp, minimum_divisions, int, dimensions))
				return 0;
			minimum_divisions = temp;
		}
		else if (dimensions == minimum_divisions_size)
		{
			bool no_change = true;
			for (int i = 0; i < dimensions; i++)
			{
				if (minimum_divisions[i] != in_minimum_divisions[i])
					no_change = false;
			}
			if (no_change)
				return 1;
		}
		minimum_divisions_size = dimensions;
		for (int i = 0; i < dimensions; i++)
		{
			minimum_divisions[i] = in_minimum_divisions[i];
		}
		this->changeDetail.setElementDivisionsChanged();
		MANAGED_OBJECT_CHANGE(Cmiss_tessellation)(this,
			MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(Cmiss_tessellation));
		return 1;
	}

	/** assumes arguments have been checked already */
	int set_refinement_factors(int dimensions, const int *in_refinement_factors)
	{
		if (dimensions > refinement_factors_size)
		{
			int *temp;
			if (!REALLOCATE(temp, refinement_factors, int, dimensions))
				return 0;
			refinement_factors = temp;
		}
		else if (dimensions == refinement_factors_size)
		{
			bool no_change = true;
			for (int i = 0; i < dimensions; i++)
			{
				if (refinement_factors[i] != in_refinement_factors[i])
					no_change = false;
			}
			if (no_change)
				return 1;
		}
		refinement_factors_size = dimensions;
		for (int i = 0; i < dimensions; i++)
		{
			refinement_factors[i] =  in_refinement_factors[i];
		}
		this->changeDetail.setElementDivisionsChanged();
		MANAGED_OBJECT_CHANGE(Cmiss_tessellation)(this,
			MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(Cmiss_tessellation));
		return 1;
	}

	void list()
	{
		display_message(INFORMATION_MESSAGE, "gfx define tessellation %s minimum_divisions \"", name);
		if (minimum_divisions_size)
		{
			list_divisions(minimum_divisions_size, minimum_divisions);
		}
		else
		{
			display_message(INFORMATION_MESSAGE, "1");
		}
		display_message(INFORMATION_MESSAGE, "\" refinement_factors \"");
		if (refinement_factors_size)
		{
			list_divisions(refinement_factors_size, refinement_factors);
		}
		else
		{
			display_message(INFORMATION_MESSAGE, "1");
		}
		display_message(INFORMATION_MESSAGE, "\" circle_divisions %d;\n", circleDivisions);
	}

	inline Cmiss_tessellation *access()
	{
		++access_count;
		return this;
	}

	/** deaccess handling is_managed_flag */
	static inline int deaccess(Cmiss_tessellation **object_address)
	{
		int return_code = 1;
		Cmiss_tessellation *object;
		if (object_address && (object = *object_address))
		{
			(object->access_count)--;
			if (object->access_count <= 0)
			{
				delete object;
				object = 0;
			}
			else if ((!object->is_managed_flag) && (object->manager) &&
				((1 == object->access_count) || ((2 == object->access_count) &&
					(MANAGER_CHANGE_NONE(Cmiss_tessellation) != object->manager_change_status))))
			{
				return_code = REMOVE_OBJECT_FROM_MANAGER(Cmiss_tessellation)(object, object->manager);
			}
			*object_address = static_cast<Cmiss_tessellation *>(0);
		}
		else
		{
			return_code = 0;
		}
		return (return_code);
	}

}; /* struct Cmiss_tessellation */

/* Only to be used from FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL function
 * Creates a pseudo object with name identifier suitable for finding
 * objects by identifier with Cmiss_set.
 */
class Cmiss_tessellation_identifier : private Cmiss_tessellation
{
public:
	Cmiss_tessellation_identifier(const char *name)
	{
		Cmiss_tessellation::name = name;
	}

	~Cmiss_tessellation_identifier()
	{
		Cmiss_tessellation::name = NULL;
	}

	Cmiss_tessellation *getPseudoObject()
	{
		return this;
	}
};

/** functor for ordering Cmiss_set<Cmiss_tessellation> by name */
struct Cmiss_tessellation_compare_name
{
	bool operator() (const Cmiss_tessellation* tessellation1, const Cmiss_tessellation* tessellation2) const
	{
		return strcmp(tessellation1->name, tessellation2->name) < 0;
	}
};

typedef Cmiss_set<Cmiss_tessellation *,Cmiss_tessellation_compare_name> Cmiss_set_Cmiss_tessellation;

FULL_DECLARE_MANAGER_TYPE_WITH_OWNER(Cmiss_tessellation, Cmiss_tessellation_module, Cmiss_tessellation_change_detail *);

/*
Module functions
----------------
*/

namespace {

DECLARE_DEFAULT_MANAGER_UPDATE_DEPENDENCIES_FUNCTION(Cmiss_tessellation)

/** Override to extract type-specific change details from field */
static inline Cmiss_tessellation_change_detail *MANAGER_EXTRACT_CHANGE_DETAIL(Cmiss_tessellation)(
	struct Cmiss_tessellation *tessellation)
{
	return tessellation->extractChangeDetail();
}

DECLARE_MANAGER_UPDATE_FUNCTION(Cmiss_tessellation)

DECLARE_MANAGER_FIND_CLIENT_FUNCTION(Cmiss_tessellation)

DECLARE_MANAGED_OBJECT_NOT_IN_USE_CONDITIONAL_FUNCTION(Cmiss_tessellation)

} /* anonymous namespace */

/*
Global functions
----------------
*/

PROTOTYPE_ACCESS_OBJECT_FUNCTION(Cmiss_tessellation)
{
	if (object)
		return object->access();
	return 0;
}

PROTOTYPE_DEACCESS_OBJECT_FUNCTION(Cmiss_tessellation)
{
	return Cmiss_tessellation::deaccess(object_address);
}

PROTOTYPE_REACCESS_OBJECT_FUNCTION(Cmiss_tessellation)
{
	if (object_address)
	{
		if (new_object)
		{
			new_object->access();
		}
		if (*object_address)
		{
			Cmiss_tessellation::deaccess(object_address);
		}
		*object_address = new_object;
		return 1;
	}
	return 0;
}

DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(Cmiss_tessellation)

DECLARE_INDEXED_LIST_STL_FUNCTIONS(Cmiss_tessellation)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL_FUNCTION(Cmiss_tessellation,name,const char *)

DECLARE_MANAGER_FUNCTIONS(Cmiss_tessellation,manager)
DECLARE_DEFAULT_MANAGED_OBJECT_NOT_IN_USE_FUNCTION(Cmiss_tessellation,manager)
DECLARE_MANAGER_IDENTIFIER_WITHOUT_MODIFY_FUNCTIONS(Cmiss_tessellation,name,const char *,manager)
DECLARE_MANAGER_OWNER_FUNCTIONS(Cmiss_tessellation, struct Cmiss_tessellation_module)

int Cmiss_tessellation_manager_set_owner_private(struct MANAGER(Cmiss_tessellation) *manager,
	struct Cmiss_tessellation_module *tessellation_module)
{
	return MANAGER_SET_OWNER(Cmiss_tessellation)(manager, tessellation_module);
}

int Cmiss_tessellation_manager_message_get_object_change_and_detail(
	struct MANAGER_MESSAGE(Cmiss_tessellation) *message, Cmiss_tessellation *tessellation,
	const Cmiss_tessellation_change_detail **change_detail_address)
{
	if (message && tessellation && change_detail_address)
	{
		int i;
		struct MANAGER_MESSAGE_OBJECT_CHANGE(Cmiss_tessellation) *object_change;

		object_change = message->object_changes;
		for (i = message->number_of_changed_objects; 0 < i; i--)
		{
			if (tessellation == object_change->object)
			{
				*change_detail_address = object_change->detail;
				return (object_change->change);
			}
			object_change++;
		}
	}
	if (change_detail_address)
	{
		*change_detail_address = 0;
	}
	return (MANAGER_CHANGE_NONE(Cmiss_tessellation));
}

Cmiss_tessellation_module_id Cmiss_tessellation_module_create()
{
	return Cmiss_tessellation_module::create();
}

Cmiss_tessellation_module_id Cmiss_tessellation_module_access(
	Cmiss_tessellation_module_id tessellation_module)
{
	if (tessellation_module)
		return tessellation_module->access();
	return 0;
}

int Cmiss_tessellation_module_destroy(Cmiss_tessellation_module_id *tessellation_module_address)
{
	if (tessellation_module_address)
		return Cmiss_tessellation_module::deaccess(*tessellation_module_address);
	return CMISS_ERROR_ARGUMENT;
}

Cmiss_tessellation_id Cmiss_tessellation_module_create_tessellation(
	Cmiss_tessellation_module_id tessellation_module)
{
	if (tessellation_module)
		return tessellation_module->createTessellation();
	return 0;
}

struct MANAGER(Cmiss_tessellation) *Cmiss_tessellation_module_get_manager(
	Cmiss_tessellation_module_id tessellation_module)
{
	if (tessellation_module)
		return tessellation_module->getManager();
	return 0;
}

int Cmiss_tessellation_module_begin_change(Cmiss_tessellation_module_id tessellation_module)
{
	if (tessellation_module)
		return tessellation_module->beginChange();
   return CMISS_ERROR_ARGUMENT;
}

int Cmiss_tessellation_module_end_change(Cmiss_tessellation_module_id tessellation_module)
{
	if (tessellation_module)
		return tessellation_module->endChange();
   return CMISS_ERROR_ARGUMENT;
}

Cmiss_tessellation_id Cmiss_tessellation_module_find_tessellation_by_name(
	Cmiss_tessellation_module_id tessellation_module, const char *name)
{
	if (tessellation_module)
		return tessellation_module->findTessellationByName(name);
   return 0;
}

Cmiss_tessellation_id Cmiss_tessellation_module_get_default_tessellation(
	Cmiss_tessellation_module_id tessellation_module)
{
	if (tessellation_module)
		return tessellation_module->getDefaultTessellation();
	return 0;
}

int Cmiss_tessellation_module_set_default_tessellation(
	Cmiss_tessellation_module_id tessellation_module,
	Cmiss_tessellation_id tessellation)
{
	if (tessellation_module)
		return tessellation_module->setDefaultTessellation(tessellation);
	return 0;
}

Cmiss_tessellation_id Cmiss_tessellation_module_get_default_points_tessellation(
	Cmiss_tessellation_module_id tessellation_module)
{
	if (tessellation_module)
		return tessellation_module->getDefaultPointsTessellation();
	return 0;
}

int Cmiss_tessellation_module_set_default_points_tessellation(
	Cmiss_tessellation_module_id tessellation_module,
	Cmiss_tessellation_id tessellation)
{
	if (tessellation_module)
		return tessellation_module->setDefaultPointsTessellation(tessellation);
	return 0;
}

struct Cmiss_tessellation *Cmiss_tessellation_create_private()
{
	return Cmiss_tessellation::create();
}

Cmiss_tessellation_id Cmiss_tessellation_access(Cmiss_tessellation_id tessellation)
{
	if (tessellation)
		return ACCESS(Cmiss_tessellation)(tessellation);
	return 0;
}

int Cmiss_tessellation_destroy(Cmiss_tessellation_id *tessellation_address)
{
	return DEACCESS(Cmiss_tessellation)(tessellation_address);
}

bool Cmiss_tessellation_is_managed(Cmiss_tessellation_id tessellation)
{
	if (tessellation)
	{
		return tessellation->is_managed_flag;
	}
	return 0;
}

int Cmiss_tessellation_set_managed(Cmiss_tessellation_id tessellation,
	bool value)
{
	if (tessellation)
	{
		bool old_value = tessellation->is_managed_flag;
		tessellation->is_managed_flag = (value != 0);
		if (value != old_value)
		{
			MANAGED_OBJECT_CHANGE(Cmiss_tessellation)(tessellation,
				MANAGER_CHANGE_NOT_RESULT(Cmiss_tessellation));
		}
		return CMISS_OK;
	}
	return CMISS_ERROR_ARGUMENT;
}

char *Cmiss_tessellation_get_name(struct Cmiss_tessellation *tessellation)
{
	char *name = NULL;
	if (tessellation && tessellation->name)
	{
		name = duplicate_string(tessellation->name);
	}
	return name;
}

int Cmiss_tessellation_set_name(struct Cmiss_tessellation *tessellation, const char *name)
{
	int return_code;

	ENTER(Cmiss_tessellation_set_name);
	if (tessellation && is_standard_object_name(name))
	{
		return_code = 1;
		Cmiss_set_Cmiss_tessellation *manager_tessellation_list = 0;
		bool restore_changed_object_to_lists = false;
		if (tessellation->manager)
		{
			Cmiss_tessellation *existing_tessellation =
				FIND_BY_IDENTIFIER_IN_MANAGER(Cmiss_tessellation, name)(name, tessellation->manager);
			if (existing_tessellation && (existing_tessellation != tessellation))
			{
				display_message(ERROR_MESSAGE, "Cmiss_tessellation_set_name.  "
					"tessellation named '%s' already exists.", name);
				return_code = 0;
			}
			if (return_code)
			{
				manager_tessellation_list = reinterpret_cast<Cmiss_set_Cmiss_tessellation *>(
					tessellation->manager->object_list);
				// this temporarily removes the object from all related lists
				restore_changed_object_to_lists =
					manager_tessellation_list->begin_identifier_change(tessellation);
				if (!restore_changed_object_to_lists)
				{
					display_message(ERROR_MESSAGE, "Cmiss_tessellation_set_name.  "
						"Could not safely change identifier in manager");
					return_code = 0;
				}
			}
		}
		if (return_code)
		{
			char *new_name = duplicate_string(name);
			if (new_name)
			{
				DEALLOCATE(tessellation->name);
				tessellation->name = new_name;
			}
			else
			{
				return_code = 0;
			}
		}
		if (restore_changed_object_to_lists)
		{
			manager_tessellation_list->end_identifier_change();
		}
		if (tessellation->manager && return_code)
		{
			MANAGED_OBJECT_CHANGE(Cmiss_tessellation)(tessellation,
				MANAGER_CHANGE_IDENTIFIER(Cmiss_tessellation));
		}
	}
	else
	{
		if (tessellation)
		{
			display_message(ERROR_MESSAGE,
				"Cmiss_tessellation_set_name.  Invalid tessellation name '%s'", name);
		}
		return_code=0;
	}

	return (return_code);
}

int Cmiss_tessellation_get_circle_divisions(
	Cmiss_tessellation_id tessellation)
{
	if (tessellation)
		return tessellation->getCircleDivisions();
	return 0;
}

int Cmiss_tessellation_set_circle_divisions(
	Cmiss_tessellation_id tessellation, int circleDivisions)
{
	if (tessellation)
		return tessellation->setCircleDivisions(circleDivisions);
	return CMISS_ERROR_ARGUMENT;
}

int Cmiss_tessellation_get_minimum_divisions(Cmiss_tessellation_id tessellation,
	int valuesCount, int *valuesOut)
{
	if (tessellation && ((valuesCount == 0) || ((valuesCount > 0) && valuesOut)))
	{
		for (int i = 0; i < valuesCount; i++)
		{
			valuesOut[i] = tessellation->get_minimum_divisions_value(i);
		}
		return tessellation->minimum_divisions_size;
	}
	return 0;
}

int Cmiss_tessellation_set_minimum_divisions(Cmiss_tessellation_id tessellation,
	int valuesCount, const int *valuesIn)
{
	if (tessellation && (valuesCount > 0) && valuesIn)
	{
		for (int i = 0; i < valuesCount; i++)
		{
			if (valuesIn[i] < 1)
			{
				return CMISS_ERROR_ARGUMENT;
			}
		}
		tessellation->set_minimum_divisions(valuesCount, valuesIn);
		return CMISS_OK;
	}
	return CMISS_ERROR_ARGUMENT;
}

int Cmiss_tessellation_get_refinement_factors(Cmiss_tessellation_id tessellation,
	int valuesCount, int *valuesOut)
{
	if (tessellation && ((valuesCount == 0) || ((valuesCount > 0) && valuesOut)))
	{
		for (int i = 0; i < valuesCount; i++)
		{
			valuesOut[i] = tessellation->get_refinement_factors_value(i);
		}
		return tessellation->refinement_factors_size;
	}
	return 0;
}

int Cmiss_tessellation_set_refinement_factors(Cmiss_tessellation_id tessellation,
	int valuesCount, const int *valuesIn)
{
	if (tessellation && (valuesCount > 0) && valuesIn)
	{
		for (int i = 0; i < valuesCount; i++)
		{
			if (valuesIn[i] < 1)
			{
				return CMISS_ERROR_ARGUMENT;
			}
		}
		tessellation->set_refinement_factors(valuesCount, valuesIn);
		return CMISS_OK;
	}
	return CMISS_ERROR_ARGUMENT;
}

Cmiss_tessellation_id Cmiss_tessellation_module_find_or_create_fixed_tessellation(
	Cmiss_tessellation_module_id tessellationModule,
	int elementDivisionsCount, int *elementDivisions, int circleDivisions,
	Cmiss_tessellation_id defaultTessellation)
{
	Cmiss_tessellation_id tessellation = 0;
	if (tessellationModule && ((0 == elementDivisionsCount) ||
		((0 < elementDivisionsCount && elementDivisions))) && defaultTessellation)
	{
		Cmiss_set_Cmiss_tessellation *all_tessellations =
			reinterpret_cast<Cmiss_set_Cmiss_tessellation *>(tessellationModule->getManager()->object_list);
		for (Cmiss_set_Cmiss_tessellation::iterator iter = all_tessellations->begin();
			iter != all_tessellations->end(); ++iter)
		{
			Cmiss_tessellation_id tempTessellation = *iter;
			bool match = (0 == circleDivisions) || (tempTessellation->circleDivisions == circleDivisions);
			if (match && (0 < elementDivisionsCount))
			{
				int count = elementDivisionsCount;
				if (count < tempTessellation->minimum_divisions_size)
					count = tempTessellation->minimum_divisions_size;
				if (count < tempTessellation->refinement_factors_size)
					count = tempTessellation->refinement_factors_size;
				int value = 0;
				for (int i = 0; i < count; i++)
				{
					if (i < elementDivisionsCount)
						value = elementDivisions[i];
					if ((tempTessellation->get_minimum_divisions_value(i) != value) ||
						(tempTessellation->get_refinement_factors_value(i) != 1))
					{
						match = false;
						break;
					}
				}
			}
			if (match)
			{
				tessellation = tempTessellation->access();
				break;
			}
		}
		if (!tessellation)
		{
			tessellationModule->beginChange();
			tessellation = tessellationModule->createTessellation();
			*tessellation = *defaultTessellation;
			if (0 < elementDivisionsCount)
			{
				tessellation->set_minimum_divisions(elementDivisionsCount, elementDivisions);
				const int one = 1;
				tessellation->set_refinement_factors(1, &one);
			}
			if (0 != circleDivisions)
			{
				tessellation->setCircleDivisions(circleDivisions);
			}
			tessellationModule->endChange();
		}	
	}
	return tessellation;
}

int string_to_divisions(const char *input, int **values_in, int *size_in)
{
	int return_code = 1;
	int *values = NULL;
	const char *str = input;
	int size = 0;
	while (input)
	{
		char *end = NULL;
		int value = (int)strtol(str, &end, /*base*/10);
		if (value <= 0)
		{
			display_message(ERROR_MESSAGE,
					"Non-positive or missing integer in string: %s", input);
			return_code = 0;
			break;
		}
		while (*end == ' ')
		{
			end++;
		}
		size++;
		int *temp;
		if (!REALLOCATE(temp, values, int, size))
		{
			return_code = 0;
			break;
		}
		values = temp;
		values[size - 1] = value;
		if (*end == '\0')
		{
			break;
		}
		if (*end == '*')
		{
			end++;
		}
		else
		{
			display_message(ERROR_MESSAGE,
					"Invalid character \'%c' where * expected", *end);
			return_code = 0;
			break;
		}
		str = end;
	}
	*size_in = size;
	*values_in = values;

	return return_code;
}

int list_Cmiss_tessellation_iterator(struct Cmiss_tessellation *tessellation, void *dummy_void)
{
	USE_PARAMETER(dummy_void);
	if (tessellation)
	{
		tessellation->list();
		return 1;
	}
	return 0;
}
