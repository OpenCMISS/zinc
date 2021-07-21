/***************************************************************************//**
 * tessellation.cpp
 *
 * Objects for describing how elements / continuous field domains are
 * tessellated or sampled into graphics.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <iterator>
#include <list>
#include <cstdlib>
#include "opencmiss/zinc/status.h"
#include "description_io/tessellation_json_io.hpp"
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
typedef std::list<cmzn_tessellationmodulenotifier *> cmzn_tessellationmodulenotifier_list;

struct cmzn_tessellation *cmzn_tessellation_create_private();

static void cmzn_tessellationmodule_Tessellation_change(
	struct MANAGER_MESSAGE(cmzn_tessellation) *message, void *tessellationmodule_void);

struct cmzn_tessellationmodule
{

private:

	void *manager_callback_id;
	struct MANAGER(cmzn_tessellation) *tessellationManager;
	cmzn_tessellation *defaultTessellation;
	cmzn_tessellation *defaultPointsTessellation;
	int access_count;

	cmzn_tessellationmodule() :
		tessellationManager(CREATE(MANAGER(cmzn_tessellation))()),
		defaultTessellation(0),
		defaultPointsTessellation(0),
		access_count(1)
	{
		notifier_list = new cmzn_tessellationmodulenotifier_list();
		manager_callback_id = MANAGER_REGISTER(cmzn_tessellation)(
			cmzn_tessellationmodule_Tessellation_change, (void *)this, tessellationManager);
	}

	~cmzn_tessellationmodule()
	{
		MANAGER_DEREGISTER(cmzn_tessellation)(manager_callback_id, tessellationManager);
		manager_callback_id = 0;
		for (cmzn_tessellationmodulenotifier_list::iterator iter = notifier_list->begin();
			iter != notifier_list->end(); ++iter)
		{
			cmzn_tessellationmodulenotifier *notifier = *iter;
			notifier->tessellationmoduleDestroyed();
			cmzn_tessellationmodulenotifier::deaccess(notifier);
		}
		delete notifier_list;
		notifier_list = 0;
		cmzn_tessellation_destroy(&this->defaultTessellation);
		cmzn_tessellation_destroy(&this->defaultPointsTessellation);
		DESTROY(MANAGER(cmzn_tessellation))(&(this->tessellationManager));
	}

public:

	cmzn_tessellationmodulenotifier_list *notifier_list;

	static cmzn_tessellationmodule *create()
	{
		cmzn_tessellationmodule *tessellationModule = new cmzn_tessellationmodule();
		cmzn_tessellation_id tessellation = tessellationModule->getDefaultTessellation();
		cmzn_tessellation_id pointTessellation = tessellationModule->getDefaultPointsTessellation();
		cmzn_tessellation_destroy(&tessellation);
		cmzn_tessellation_destroy(&pointTessellation);
		return tessellationModule;
	}

	cmzn_tessellationmodule *access()

	{
		++access_count;
		return this;
	}

	static int deaccess(cmzn_tessellationmodule* &tessellationmodule)
	{
		if (tessellationmodule)
		{
			--(tessellationmodule->access_count);
			if (tessellationmodule->access_count <= 0)
			{
				delete tessellationmodule;
			}
			tessellationmodule = 0;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	struct MANAGER(cmzn_tessellation) *getManager()
	{
		return this->tessellationManager;
	}

	int beginChange()
	{
		return MANAGER_BEGIN_CACHE(cmzn_tessellation)(this->tessellationManager);
	}

	int endChange()
	{
		return MANAGER_END_CACHE(cmzn_tessellation)(this->tessellationManager);
	}

	cmzn_tessellation_id createTessellation()
	{
		cmzn_tessellation_id tessellation = NULL;
		char temp_name[20];
		int i = NUMBER_IN_MANAGER(cmzn_tessellation)(this->tessellationManager);
		do
		{
			i++;
			sprintf(temp_name, "temp%d",i);
		}
		while (FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_tessellation,name)(temp_name,
			this->tessellationManager));
		tessellation = cmzn_tessellation_create_private();
		cmzn_tessellation_set_name(tessellation, temp_name);
		if (!ADD_OBJECT_TO_MANAGER(cmzn_tessellation)(tessellation, this->tessellationManager))
		{
			DEACCESS(cmzn_tessellation)(&tessellation);
		}
		return tessellation;
	}

	cmzn_tessellationiterator *createTessellationiterator();

	cmzn_tessellation *findTessellationByName(const char *name)
	{
		cmzn_tessellation *tessellation = FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_tessellation,name)(name,
			this->tessellationManager);
		if (tessellation)
		{
			return ACCESS(cmzn_tessellation)(tessellation);
		}
		return 0;
	}

	cmzn_tessellation *getDefaultTessellation()
	{
		if (this->defaultTessellation)
		{
			ACCESS(cmzn_tessellation)(this->defaultTessellation);
		}
		else
		{
			this->beginChange();
			cmzn_tessellation *tessellation = createTessellation();
			cmzn_tessellation_set_name(tessellation, "default");
			const int default_minimum_divisions = 1;
			cmzn_tessellation_set_minimum_divisions(tessellation,
				/*dimensions*/1, &default_minimum_divisions);
			const int default_refinement_factor = 6;
			cmzn_tessellation_set_refinement_factors(tessellation,
				/*dimensions*/1, &default_refinement_factor);
			cmzn_tessellation_set_circle_divisions(tessellation, 12);
			this->setDefaultTessellation(tessellation);
			this->endChange();
		}
		return this->defaultTessellation;
	}

	int setDefaultTessellation(cmzn_tessellation *tessellation)
	{
		REACCESS(cmzn_tessellation)(&this->defaultTessellation, tessellation);
		return CMZN_OK;
	}

	cmzn_tessellation *getDefaultPointsTessellation()
	{
		if (this->defaultPointsTessellation)
		{
			ACCESS(cmzn_tessellation)(this->defaultPointsTessellation);
		}
		else
		{
			this->beginChange();
			cmzn_tessellation *tessellation = createTessellation();
			cmzn_tessellation_set_name(tessellation, "default_points");
			const int default_minimum_divisions = 1;
			cmzn_tessellation_set_minimum_divisions(tessellation,
				/*dimensions*/1, &default_minimum_divisions);
			const int default_refinement_factor = 1;
			cmzn_tessellation_set_refinement_factors(tessellation,
				/*dimensions*/1, &default_refinement_factor);
			cmzn_tessellation_set_circle_divisions(tessellation, 12);
			this->setDefaultPointsTessellation(tessellation);
			this->endChange();
		}
		return this->defaultPointsTessellation;
	}

	int setDefaultPointsTessellation(cmzn_tessellation *tessellation)
	{
		REACCESS(cmzn_tessellation)(&this->defaultPointsTessellation, tessellation);
		return CMZN_OK;
	}

	void addNotifier(cmzn_tessellationmodulenotifier *notifier)
	{
		notifier_list->push_back(notifier->access());
	}

	void removeNotifier(cmzn_tessellationmodulenotifier *notifier)
	{
		if (notifier)
		{
			cmzn_tessellationmodulenotifier_list::iterator iter = std::find(
				notifier_list->begin(), notifier_list->end(), notifier);
			if (iter != notifier_list->end())
			{
				cmzn_tessellationmodulenotifier::deaccess(notifier);
				notifier_list->erase(iter);
			}
		}
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
struct cmzn_tessellation
{
	const char *name;
	/* after clearing in create, following to be modified only by manager */
	struct MANAGER(cmzn_tessellation) *manager;
	int manager_change_status;
	int circleDivisions;
	int minimum_divisions_size;
	int *minimum_divisions;
	int refinement_factors_size;
	int *refinement_factors;
	cmzn_tessellation_change_detail changeDetail;
	bool is_managed_flag;
	int access_count;

protected:

	cmzn_tessellation() :
		name(NULL),
		manager(NULL),
		manager_change_status(MANAGER_CHANGE_NONE(cmzn_tessellation)),
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

	~cmzn_tessellation()
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
	static cmzn_tessellation *create()
	{
		return new cmzn_tessellation();
	}

	cmzn_tessellation& operator=(const cmzn_tessellation& source)
	{
		this->set_minimum_divisions(source.minimum_divisions_size, source.minimum_divisions);
		this->set_refinement_factors(source.refinement_factors_size, source.refinement_factors);
		this->setCircleDivisions(source.circleDivisions);
		return *this;
	}

	cmzn_tessellation_change_detail *extractChangeDetail()
	{
		cmzn_tessellation_change_detail *change_detail = new cmzn_tessellation_change_detail(this->changeDetail);
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
			MANAGED_OBJECT_CHANGE(cmzn_tessellation)(this,
				MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(cmzn_tessellation));
		}
		return (inCircleDivisions == this->circleDivisions) ? CMZN_OK : CMZN_ERROR_ARGUMENT;
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
		MANAGED_OBJECT_CHANGE(cmzn_tessellation)(this,
			MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(cmzn_tessellation));
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
		MANAGED_OBJECT_CHANGE(cmzn_tessellation)(this,
			MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(cmzn_tessellation));
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

	inline cmzn_tessellation *access()
	{
		++access_count;
		return this;
	}

	/** deaccess handling is_managed_flag */
	static inline int deaccess(cmzn_tessellation **object_address)
	{
		int return_code = 1;
		cmzn_tessellation *object;
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
					(MANAGER_CHANGE_NONE(cmzn_tessellation) != object->manager_change_status))))
			{
				return_code = REMOVE_OBJECT_FROM_MANAGER(cmzn_tessellation)(object, object->manager);
			}
			*object_address = static_cast<cmzn_tessellation *>(0);
		}
		else
		{
			return_code = 0;
		}
		return (return_code);
	}

}; /* struct cmzn_tessellation */

/* Only to be used from FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL function
 * Creates a pseudo object with name identifier suitable for finding
 * objects by identifier with cmzn_set.
 */
class cmzn_tessellation_identifier : private cmzn_tessellation
{
public:
	cmzn_tessellation_identifier(const char *name)
	{
		cmzn_tessellation::name = name;
	}

	~cmzn_tessellation_identifier()
	{
		cmzn_tessellation::name = NULL;
	}

	cmzn_tessellation *getPseudoObject()
	{
		return this;
	}
};

/** functor for ordering cmzn_set<cmzn_tessellation> by name */
struct cmzn_tessellation_compare_name
{
	bool operator() (const cmzn_tessellation* tessellation1, const cmzn_tessellation* tessellation2) const
	{
		return strcmp(tessellation1->name, tessellation2->name) < 0;
	}
};

typedef cmzn_set<cmzn_tessellation *,cmzn_tessellation_compare_name> cmzn_set_cmzn_tessellation;

struct cmzn_tessellationiterator : public cmzn_set_cmzn_tessellation::ext_iterator
{
private:
	cmzn_tessellationiterator(cmzn_set_cmzn_tessellation *container);
	cmzn_tessellationiterator(const cmzn_tessellationiterator&);
	~cmzn_tessellationiterator();

public:

	static cmzn_tessellationiterator *create(cmzn_set_cmzn_tessellation *container)
	{
		return static_cast<cmzn_tessellationiterator *>(cmzn_set_cmzn_tessellation::ext_iterator::create(container));
	}

	cmzn_tessellationiterator *access()
	{
		return static_cast<cmzn_tessellationiterator *>(this->cmzn_set_cmzn_tessellation::ext_iterator::access());
	}

	static int deaccess(cmzn_tessellationiterator* &iterator)
	{
		cmzn_set_cmzn_tessellation::ext_iterator* baseIterator = static_cast<cmzn_set_cmzn_tessellation::ext_iterator*>(iterator);
		iterator = 0;
		return cmzn_set_cmzn_tessellation::ext_iterator::deaccess(baseIterator);
	}

};

FULL_DECLARE_MANAGER_TYPE_WITH_OWNER(cmzn_tessellation, cmzn_tessellationmodule, cmzn_tessellation_change_detail *);

/*
Module functions
----------------
*/

DECLARE_DEFAULT_MANAGER_UPDATE_DEPENDENCIES_FUNCTION(cmzn_tessellation)

inline cmzn_tessellation_change_detail *MANAGER_EXTRACT_CHANGE_DETAIL(cmzn_tessellation)(
	struct cmzn_tessellation *tessellation)
{
	return tessellation->extractChangeDetail();
}

inline void MANAGER_CLEANUP_CHANGE_DETAIL(cmzn_tessellation)(
	cmzn_tessellation_change_detail **change_detail_address)
{
	delete *change_detail_address;
}

DECLARE_MANAGER_UPDATE_FUNCTION(cmzn_tessellation)

DECLARE_MANAGER_FIND_CLIENT_FUNCTION(cmzn_tessellation)

DECLARE_MANAGED_OBJECT_NOT_IN_USE_CONDITIONAL_FUNCTION(cmzn_tessellation)

/*
Global functions
----------------
*/

PROTOTYPE_ACCESS_OBJECT_FUNCTION(cmzn_tessellation)
{
	if (object)
		return object->access();
	return 0;
}

PROTOTYPE_DEACCESS_OBJECT_FUNCTION(cmzn_tessellation)
{
	return cmzn_tessellation::deaccess(object_address);
}

PROTOTYPE_REACCESS_OBJECT_FUNCTION(cmzn_tessellation)
{
	if (object_address)
	{
		if (new_object)
		{
			new_object->access();
		}
		if (*object_address)
		{
			cmzn_tessellation::deaccess(object_address);
		}
		*object_address = new_object;
		return 1;
	}
	return 0;
}

DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(cmzn_tessellation)

DECLARE_INDEXED_LIST_STL_FUNCTIONS(cmzn_tessellation)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL_FUNCTION(cmzn_tessellation,name,const char *)
DECLARE_INDEXED_LIST_STL_IDENTIFIER_CHANGE_FUNCTIONS(cmzn_tessellation,name) 
DECLARE_CREATE_INDEXED_LIST_STL_ITERATOR_FUNCTION(cmzn_tessellation,cmzn_tessellationiterator) 

DECLARE_MANAGER_FUNCTIONS(cmzn_tessellation,manager)
DECLARE_DEFAULT_MANAGED_OBJECT_NOT_IN_USE_FUNCTION(cmzn_tessellation,manager)
DECLARE_MANAGER_IDENTIFIER_WITHOUT_MODIFY_FUNCTIONS(cmzn_tessellation,name,const char *,manager)
DECLARE_MANAGER_OWNER_FUNCTIONS(cmzn_tessellation, struct cmzn_tessellationmodule)

cmzn_tessellationiterator *cmzn_tessellationmodule::createTessellationiterator()
{
	return CREATE_LIST_ITERATOR(cmzn_tessellation)(this->tessellationManager->object_list);
}

int cmzn_tessellation_manager_set_owner_private(struct MANAGER(cmzn_tessellation) *manager,
	struct cmzn_tessellationmodule *tessellationmodule)
{
	return MANAGER_SET_OWNER(cmzn_tessellation)(manager, tessellationmodule);
}

int cmzn_tessellation_manager_message_get_object_change_and_detail(
	struct MANAGER_MESSAGE(cmzn_tessellation) *message, cmzn_tessellation *tessellation,
	const cmzn_tessellation_change_detail **change_detail_address)
{
	if (message)
		return message->getObjectChangeFlagsAndDetail(tessellation, change_detail_address);
	if (change_detail_address)
		*change_detail_address = 0;
	return (MANAGER_CHANGE_NONE(cmzn_tessellation));
}

/**
 * Tessellation manager callback. Calls notifier callbacks.
 *
 * @param message  The changes to the tessellations in the tessellation manager.
 * @param tessellationmodule_void  Void pointer to changed tessellationmodule).
 */
static void cmzn_tessellationmodule_Tessellation_change(
	struct MANAGER_MESSAGE(cmzn_tessellation) *message, void *tessellationmodule_void)
{
	cmzn_tessellationmodule *tessellationmodule = (cmzn_tessellationmodule *)tessellationmodule_void;
	if (message && tessellationmodule)
	{
		int change_summary = MANAGER_MESSAGE_GET_CHANGE_SUMMARY(cmzn_tessellation)(message);

		if (0 < tessellationmodule->notifier_list->size())
		{
			cmzn_tessellationmoduleevent_id event = cmzn_tessellationmoduleevent::create(tessellationmodule);
			event->setChangeFlags(change_summary);
			event->setManagerMessage(message);
			for (cmzn_tessellationmodulenotifier_list::iterator iter =
				tessellationmodule->notifier_list->begin();
				iter != tessellationmodule->notifier_list->end(); ++iter)
			{
				(*iter)->notify(event);
			}
			cmzn_tessellationmoduleevent::deaccess(event);
		}
	}
}


cmzn_tessellationmodulenotifier_id cmzn_tessellationmodule_create_tessellationmodulenotifier(
	cmzn_tessellationmodule_id tessellationmodule)
{
	return cmzn_tessellationmodulenotifier::create(tessellationmodule);
}

cmzn_tessellationmodulenotifier::cmzn_tessellationmodulenotifier(
	cmzn_tessellationmodule *tessellationmodule) :
	module(tessellationmodule),
	function(0),
	user_data(0),
	access_count(1)
{
	tessellationmodule->addNotifier(this);
}

cmzn_tessellationmodulenotifier::~cmzn_tessellationmodulenotifier()
{
}

int cmzn_tessellationmodulenotifier::deaccess(cmzn_tessellationmodulenotifier* &notifier)
{
	if (notifier)
	{
		--(notifier->access_count);
		if (notifier->access_count <= 0)
			delete notifier;
		else if ((1 == notifier->access_count) && notifier->module)
			notifier->module->removeNotifier(notifier);
		notifier = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_tessellationmodulenotifier::setCallback(cmzn_tessellationmodulenotifier_callback_function function_in,
	void *user_data_in)
{
	if (!function_in)
		return CMZN_ERROR_ARGUMENT;
	this->function = function_in;
	this->user_data = user_data_in;
	return CMZN_OK;
}

void cmzn_tessellationmodulenotifier::clearCallback()
{
	this->function = 0;
	this->user_data = 0;
}

void cmzn_tessellationmodulenotifier::tessellationmoduleDestroyed()
{
	this->module = 0;
	if (this->function)
	{
		cmzn_tessellationmoduleevent_id event = cmzn_tessellationmoduleevent::create(
			static_cast<cmzn_tessellationmodule*>(0));
		event->setChangeFlags(CMZN_TESSELLATION_CHANGE_FLAG_FINAL);
		(this->function)(event, this->user_data);
		cmzn_tessellationmoduleevent::deaccess(event);
		this->clearCallback();
	}
}

cmzn_tessellationmoduleevent::cmzn_tessellationmoduleevent(
	cmzn_tessellationmodule *tessellationmoduleIn) :
	module(cmzn_tessellationmodule_access(tessellationmoduleIn)),
	changeFlags(CMZN_TESSELLATION_CHANGE_FLAG_NONE),
	managerMessage(0),
	access_count(1)
{
}

cmzn_tessellationmoduleevent::~cmzn_tessellationmoduleevent()
{
	if (managerMessage)
		MANAGER_MESSAGE_DEACCESS(cmzn_tessellation)(&(this->managerMessage));
	cmzn_tessellationmodule_destroy(&this->module);
}

cmzn_tessellation_change_flags cmzn_tessellationmoduleevent::getTessellationChangeFlags(
	cmzn_tessellation *tessellation) const
{
	if (tessellation && this->managerMessage)
		return MANAGER_MESSAGE_GET_OBJECT_CHANGE(cmzn_tessellation)(this->managerMessage, tessellation);
	return CMZN_TESSELLATION_CHANGE_FLAG_NONE;
}

void cmzn_tessellationmoduleevent::setManagerMessage(
	struct MANAGER_MESSAGE(cmzn_tessellation) *managerMessageIn)
{
	this->managerMessage = MANAGER_MESSAGE_ACCESS(cmzn_tessellation)(managerMessageIn);
}

struct MANAGER_MESSAGE(cmzn_tessellation) *cmzn_tessellationmoduleevent::getManagerMessage()
{
	return this->managerMessage;
}

int cmzn_tessellationmoduleevent::deaccess(cmzn_tessellationmoduleevent* &event)
{
	if (event)
	{
		--(event->access_count);
		if (event->access_count <= 0)
			delete event;
		event = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_tessellationmodulenotifier_clear_callback(
	cmzn_tessellationmodulenotifier_id notifier)
{
	if (notifier)
	{
		notifier->clearCallback();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_tessellationmodulenotifier_set_callback(cmzn_tessellationmodulenotifier_id notifier,
	cmzn_tessellationmodulenotifier_callback_function function_in, void *user_data_in)
{
	if (notifier && function_in)
		return notifier->setCallback(function_in, user_data_in);
	return CMZN_ERROR_ARGUMENT;
}

void *cmzn_tessellationmodulenotifier_get_callback_user_data(
 cmzn_tessellationmodulenotifier_id notifier)
{
	if (notifier)
		return notifier->getUserData();
	return 0;
}

cmzn_tessellationmodulenotifier_id cmzn_tessellationmodulenotifier_access(
	cmzn_tessellationmodulenotifier_id notifier)
{
	if (notifier)
		return notifier->access();
	return 0;
}

int cmzn_tessellationmodulenotifier_destroy(cmzn_tessellationmodulenotifier_id *notifier_address)
{
	return cmzn_tessellationmodulenotifier::deaccess(*notifier_address);
}

cmzn_tessellationmoduleevent_id cmzn_tessellationmoduleevent_access(
	cmzn_tessellationmoduleevent_id event)
{
	if (event)
		return event->access();
	return 0;
}

int cmzn_tessellationmoduleevent_destroy(cmzn_tessellationmoduleevent_id *event_address)
{
	return cmzn_tessellationmoduleevent::deaccess(*event_address);
}

cmzn_tessellation_change_flags cmzn_tessellationmoduleevent_get_summary_tessellation_change_flags(
	cmzn_tessellationmoduleevent_id event)
{
	if (event)
		return event->getChangeFlags();
	return CMZN_TESSELLATION_CHANGE_FLAG_NONE;
}

cmzn_tessellation_change_flags cmzn_tessellationmoduleevent_get_tessellation_change_flags(
	cmzn_tessellationmoduleevent_id event, cmzn_tessellation_id tessellation)
{
	if (event)
		return event->getTessellationChangeFlags(tessellation);
	return CMZN_TESSELLATION_CHANGE_FLAG_NONE;
}

cmzn_tessellationmodule_id cmzn_tessellationmodule_create()
{
	return cmzn_tessellationmodule::create();
}

cmzn_tessellationmodule_id cmzn_tessellationmodule_access(
	cmzn_tessellationmodule_id tessellationmodule)
{
	if (tessellationmodule)
		return tessellationmodule->access();
	return 0;
}

int cmzn_tessellationmodule_destroy(cmzn_tessellationmodule_id *tessellationmodule_address)
{
	if (tessellationmodule_address)
		return cmzn_tessellationmodule::deaccess(*tessellationmodule_address);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_tessellation_id cmzn_tessellationmodule_create_tessellation(
	cmzn_tessellationmodule_id tessellationmodule)
{
	if (tessellationmodule)
		return tessellationmodule->createTessellation();
	return 0;
}

cmzn_tessellationiterator_id cmzn_tessellationmodule_create_tessellationiterator(
	cmzn_tessellationmodule_id tessellationmodule)
{
	if (tessellationmodule)
		return tessellationmodule->createTessellationiterator();
	return 0;
}

struct MANAGER(cmzn_tessellation) *cmzn_tessellationmodule_get_manager(
	cmzn_tessellationmodule_id tessellationmodule)
{
	if (tessellationmodule)
		return tessellationmodule->getManager();
	return 0;
}

int cmzn_tessellationmodule_begin_change(cmzn_tessellationmodule_id tessellationmodule)
{
	if (tessellationmodule)
		return tessellationmodule->beginChange();
   return CMZN_ERROR_ARGUMENT;
}

int cmzn_tessellationmodule_end_change(cmzn_tessellationmodule_id tessellationmodule)
{
	if (tessellationmodule)
		return tessellationmodule->endChange();
   return CMZN_ERROR_ARGUMENT;
}

cmzn_tessellation_id cmzn_tessellationmodule_find_tessellation_by_name(
	cmzn_tessellationmodule_id tessellationmodule, const char *name)
{
	if (tessellationmodule)
		return tessellationmodule->findTessellationByName(name);
   return 0;
}

cmzn_tessellation_id cmzn_tessellationmodule_get_default_tessellation(
	cmzn_tessellationmodule_id tessellationmodule)
{
	if (tessellationmodule)
		return tessellationmodule->getDefaultTessellation();
	return 0;
}

int cmzn_tessellationmodule_set_default_tessellation(
	cmzn_tessellationmodule_id tessellationmodule,
	cmzn_tessellation_id tessellation)
{
	if (tessellationmodule)
		return tessellationmodule->setDefaultTessellation(tessellation);
	return 0;
}

cmzn_tessellation_id cmzn_tessellationmodule_get_default_points_tessellation(
	cmzn_tessellationmodule_id tessellationmodule)
{
	if (tessellationmodule)
		return tessellationmodule->getDefaultPointsTessellation();
	return 0;
}

int cmzn_tessellationmodule_set_default_points_tessellation(
	cmzn_tessellationmodule_id tessellationmodule,
	cmzn_tessellation_id tessellation)
{
	if (tessellationmodule)
		return tessellationmodule->setDefaultPointsTessellation(tessellation);
	return 0;
}

struct cmzn_tessellation *cmzn_tessellation_create_private()
{
	return cmzn_tessellation::create();
}

cmzn_tessellation_id cmzn_tessellation_access(cmzn_tessellation_id tessellation)
{
	if (tessellation)
		return ACCESS(cmzn_tessellation)(tessellation);
	return 0;
}

int cmzn_tessellation_destroy(cmzn_tessellation_id *tessellation_address)
{
	return DEACCESS(cmzn_tessellation)(tessellation_address);
}

bool cmzn_tessellation_is_managed(cmzn_tessellation_id tessellation)
{
	if (tessellation)
	{
		return tessellation->is_managed_flag;
	}
	return 0;
}

int cmzn_tessellation_set_managed(cmzn_tessellation_id tessellation,
	bool value)
{
	if (tessellation)
	{
		bool old_value = tessellation->is_managed_flag;
		tessellation->is_managed_flag = (value != 0);
		if (value != old_value)
		{
			MANAGED_OBJECT_CHANGE(cmzn_tessellation)(tessellation,
				MANAGER_CHANGE_DEFINITION(cmzn_tessellation));
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_tessellation_get_name(struct cmzn_tessellation *tessellation)
{
	char *name = NULL;
	if (tessellation && tessellation->name)
	{
		name = duplicate_string(tessellation->name);
	}
	return name;
}

int cmzn_tessellation_set_name(struct cmzn_tessellation *tessellation, const char *name)
{
	int return_code;

	ENTER(cmzn_tessellation_set_name);
	if (tessellation && name)
	{
		return_code = 1;
		cmzn_set_cmzn_tessellation *manager_tessellation_list = 0;
		bool restore_changed_object_to_lists = false;
		if (tessellation->manager)
		{
			cmzn_tessellation *existing_tessellation =
				FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_tessellation, name)(name, tessellation->manager);
			if (existing_tessellation && (existing_tessellation != tessellation))
			{
				display_message(ERROR_MESSAGE, "cmzn_tessellation_set_name.  "
					"tessellation named '%s' already exists.", name);
				return_code = 0;
			}
			if (return_code)
			{
				manager_tessellation_list = reinterpret_cast<cmzn_set_cmzn_tessellation *>(
					tessellation->manager->object_list);
				// this temporarily removes the object from all related lists
				restore_changed_object_to_lists =
					manager_tessellation_list->begin_identifier_change(tessellation);
				if (!restore_changed_object_to_lists)
				{
					display_message(ERROR_MESSAGE, "cmzn_tessellation_set_name.  "
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
			MANAGED_OBJECT_CHANGE(cmzn_tessellation)(tessellation,
				MANAGER_CHANGE_IDENTIFIER(cmzn_tessellation));
		}
	}
	else
	{
		if (tessellation)
		{
			display_message(ERROR_MESSAGE,
				"cmzn_tessellation_set_name.  Invalid tessellation name '%s'", name);
		}
		return_code=0;
	}

	return (return_code);
}

int cmzn_tessellation_get_circle_divisions(
	cmzn_tessellation_id tessellation)
{
	if (tessellation)
		return tessellation->getCircleDivisions();
	return 0;
}

int cmzn_tessellation_set_circle_divisions(
	cmzn_tessellation_id tessellation, int circleDivisions)
{
	if (tessellation)
		return tessellation->setCircleDivisions(circleDivisions);
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_tessellation_get_minimum_divisions(cmzn_tessellation_id tessellation,
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

int cmzn_tessellation_set_minimum_divisions(cmzn_tessellation_id tessellation,
	int valuesCount, const int *valuesIn)
{
	if (tessellation && (valuesCount > 0) && valuesIn)
	{
		for (int i = 0; i < valuesCount; i++)
		{
			if (valuesIn[i] < 1)
			{
				return CMZN_ERROR_ARGUMENT;
			}
		}
		tessellation->set_minimum_divisions(valuesCount, valuesIn);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_tessellation_get_refinement_factors(cmzn_tessellation_id tessellation,
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

int cmzn_tessellation_set_refinement_factors(cmzn_tessellation_id tessellation,
	int valuesCount, const int *valuesIn)
{
	if (tessellation && (valuesCount > 0) && valuesIn)
	{
		for (int i = 0; i < valuesCount; i++)
		{
			if (valuesIn[i] < 1)
			{
				return CMZN_ERROR_ARGUMENT;
			}
		}
		tessellation->set_refinement_factors(valuesCount, valuesIn);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_tessellation_id cmzn_tessellationmodule_find_or_create_fixed_tessellation(
	cmzn_tessellationmodule_id tessellationmodule,
	int elementDivisionsCount, int *elementDivisions, int circleDivisions,
	cmzn_tessellation_id fallbackTessellation)
{
	cmzn_tessellation_id tessellation = 0;
	if (tessellationmodule && ((0 == elementDivisionsCount) ||
		(((0 < elementDivisionsCount) && elementDivisions))) &&
		((0 == circleDivisions) || (3 <= circleDivisions)))
	{
		int oneArray[1] = { 1 };
		int useElementDivisionsCount = elementDivisionsCount;
		int* useElementDivisions = elementDivisions;
		if (0 == elementDivisionsCount)
		{
			if (fallbackTessellation)
			{
				useElementDivisionsCount = fallbackTessellation->minimum_divisions_size;
				useElementDivisions = fallbackTessellation->minimum_divisions;
			}
			else
			{
				useElementDivisionsCount = 1;
				useElementDivisions = oneArray;
			}
		}
		int useRefinementFactorsCount = 1;
		const int *useRefinementFactors = oneArray;
		int useCircleDivisions = circleDivisions;
		if (0 == circleDivisions)
		{
			if (fallbackTessellation)
				useCircleDivisions = fallbackTessellation->circleDivisions;
			else
				useCircleDivisions = 12;
		}
		cmzn_set_cmzn_tessellation *all_tessellations =
			reinterpret_cast<cmzn_set_cmzn_tessellation *>(tessellationmodule->getManager()->object_list);
		cmzn_tessellation *default_points_tessellation =
			cmzn_tessellationmodule_get_default_points_tessellation(tessellationmodule);
		for (cmzn_set_cmzn_tessellation::iterator iter = all_tessellations->begin();
			iter != all_tessellations->end(); ++iter)
		{
			cmzn_tessellation_id tempTessellation = *iter;
			// don't want to use default_points tessellation
			if (tempTessellation == default_points_tessellation)
				continue;
			bool match = (tempTessellation->circleDivisions == useCircleDivisions);
			if (match)
			{
				int count = useElementDivisionsCount;
				if (count < useRefinementFactorsCount)
					count = useRefinementFactorsCount;
				if (count < tempTessellation->minimum_divisions_size)
					count = tempTessellation->minimum_divisions_size;
				if (count < tempTessellation->refinement_factors_size)
					count = tempTessellation->refinement_factors_size;
				int divisionsValue = 1;
				int refinementValue = 1;
				for (int i = 0; i < count; i++)
				{
					if (i < useElementDivisionsCount)
						divisionsValue = useElementDivisions[i];
					if (tempTessellation->get_minimum_divisions_value(i) != divisionsValue)
					{
						match = false;
						break;
					}
					if (i < useRefinementFactorsCount)
						refinementValue = useRefinementFactors[i];
					if (tempTessellation->get_refinement_factors_value(i) != refinementValue)
					{
						match = false;
						break;
					}
				}
				if (match)
				{
					tessellation = tempTessellation->access();
					break;
				}
			}
		}
		cmzn_tessellation_destroy(&default_points_tessellation);
		if (!tessellation)
		{
			tessellationmodule->beginChange();
			tessellation = tessellationmodule->createTessellation();
			tessellation->set_minimum_divisions(useElementDivisionsCount, useElementDivisions);
			tessellation->set_refinement_factors(useRefinementFactorsCount, useRefinementFactors);
			tessellation->setCircleDivisions(useCircleDivisions);
			tessellationmodule->endChange();
		}
	}
	else
	{
		display_message(INFORMATION_MESSAGE,
			"cmzn_tessellationmodule_find_or_create_fixed_tessellation.  Invalid argument(s).");
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
			DEALLOCATE(values);
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

int list_cmzn_tessellation(struct cmzn_tessellation *tessellation)
{
	if (tessellation)
	{
		tessellation->list();
		return 1;
	}
	return 0;
}

cmzn_tessellationiterator_id cmzn_tessellationiterator_access(cmzn_tessellationiterator_id iterator)
{
	if (iterator)
		return iterator->access();
	return 0;
}

int cmzn_tessellationiterator_destroy(cmzn_tessellationiterator_id *iterator_address)
{
	if (!iterator_address)
		return 0;
	return cmzn_tessellationiterator::deaccess(*iterator_address);
}

cmzn_tessellation_id cmzn_tessellationiterator_next(cmzn_tessellationiterator_id iterator)
{
	if (iterator)
		return iterator->next();
	return 0;
}

cmzn_tessellation_id cmzn_tessellationiterator_next_non_access(cmzn_tessellationiterator_id iterator)
{
	if (iterator)
		return iterator->next_non_access();
	return 0;
}


int cmzn_tessellationmodule_read_description(cmzn_tessellationmodule_id tessellationmodule,
	const char *description)
{
	if (tessellationmodule && description)
	{
		TessellationmoduleJsonImport jsonImport(tessellationmodule);
		std::string inputString(description);
		return jsonImport.import(inputString);
	}
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_tessellationmodule_write_description(cmzn_tessellationmodule_id tessellationmodule)
{
	if (tessellationmodule)
	{
		TessellationmoduleJsonExport jsonExport(tessellationmodule);
		return duplicate_string(jsonExport.getExportString().c_str());
	}
	return 0;
}
