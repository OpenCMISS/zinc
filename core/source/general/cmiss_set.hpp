/***************************************************************************//**
 * FILE : cmiss_set.hpp
 *
 * Template class derived from STL set which handles reference counting and
 * maintains connections between sets of related objects to safely and
 * efficiently manage changing the identifier i.e. sort key of the objects.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CMZN_SET_HPP)
#define CMZN_SET_HPP

#include <set>
#include "opencmiss/zinc/status.h"

/*
Local types
-----------
*/

template<class Key, class Compare > class cmzn_set :
	private std::set<Key,Compare>
{
private:
	typedef std::set<Key,Compare> Base_class;

	mutable cmzn_set *next, *prev; // linked list of related sets
	Key temp_removed_object; // removed while changing identifier
	int access_count;

	cmzn_set() :
		next(0),
		prev(0),
		temp_removed_object(0),
		access_count(1)
	{
		next = this;
		prev = this;
	}

	/** copy constructor */
	cmzn_set(const cmzn_set& source) :
		Base_class(source),
		next(source.next),
		prev(&source),
		temp_removed_object(0),
		access_count(1)
	{
		for (iterator iter = begin(); iter != end(); ++iter)
		{
			(*iter)->access();
		}
		source.next = this;
		next->prev = this;
	}

	/** creates a set with the same manager, not a copy constructor */
	cmzn_set(const cmzn_set *source) :
		Base_class(),
		next(source->next),
		prev(const_cast<cmzn_set *>(source)),
		temp_removed_object(0),
		access_count(1)
	{
		source->next = this;
		next->prev = this;
	}

public:

	~cmzn_set()
	{
		clear();
		prev->next = next;
		next->prev = prev;
	}

	typedef typename Base_class::iterator iterator;
	typedef typename Base_class::const_iterator const_iterator;
	typedef typename Base_class::size_type size_type;

	static cmzn_set *create_independent()
	{
		return new cmzn_set();
	}

	cmzn_set *create_related() const
	{
		return new cmzn_set(this);
	}

	cmzn_set *create_copy()
	{
		return new cmzn_set(*this);
	}

	cmzn_set& operator=(const cmzn_set& source)
	{
		if (&source == this)
			return *this;
		const cmzn_set *related_set = this->next;
		while (related_set != this)
		{
			if (related_set == &source)
			{
				break;
			}
			related_set = related_set->next;
		}
		for (iterator iter = begin(); iter != end(); ++iter)
		{
			Key object = *iter;
			object->deaccess(&object);
		}
		Base_class::operator=(source);
		for (iterator iter = begin(); iter != end(); ++iter)
		{
			(*iter)->access();
		}
		if (related_set == this)
		{
			// copy from unrelated set: switch linked-lists
			this->next->prev = this->prev;
			this->prev->next = this->next;
			this->prev = const_cast<cmzn_set *>(&source);
			this->next = source.next;
			source.next->prev = this;
			source.next = this;
		}
		return *this;
	}

	inline cmzn_set *access()
	{
		++access_count;
		return this;
	}

	static inline int deaccess(cmzn_set **set_address)
	{
		if (set_address && *set_address)
		{
			if (0 >= (--(*set_address)->access_count))
			{
				delete *set_address;
			}
			*set_address = 0;
			return 1;
		}
		return 0;
	}

	size_type erase(Key object)
	{
		size_type count = Base_class::erase(object);
		if (count)
		{
			object->deaccess(&object);
		}
		return count;
	}

	void erase(iterator iter)
	{
		Key object = *iter;
		Base_class::erase(iter);
		object->deaccess(&object);
	}

	std::pair<iterator,bool> insert(Key object)
	{
		std::pair<iterator,bool> result = Base_class::insert(object);
		if (result.second)
			object->access();
		return result;
	}

	void clear()
	{
		// pessimistically avoid iterating over list in case removing one object causes the
		// list to be modified e.g. dependent fields also removed
		iterator iter;
		while ((iter = this->begin()) != this->end())
		{
			Key object = *iter;
			Base_class::erase(iter);
			object->deaccess(&object);
		}
	}

	size_type size() const
	{
		return Base_class::size();
	}

	const_iterator find(const Key &object) const
	{
		return Base_class::find(object);
	}

	iterator find(const Key &object)
	{
		return Base_class::find(object);
	}

	iterator begin()
	{
		return Base_class::begin();
	}

	const_iterator begin() const
	{
		return Base_class::begin();
	}

	iterator end()
	{
		return Base_class::end();
	}

	const_iterator end() const
	{
		return Base_class::end();
	}

	bool begin_identifier_change(Key object)
	{
		cmzn_set *related_set = this;
		do
		{
			iterator iter = related_set->find(object);
			if (iter != related_set->end())
			{
				related_set->temp_removed_object = (*iter)->access();
				related_set->erase(iter);
			}
			else
			{
				related_set->temp_removed_object = 0;
			}
			related_set = related_set->next;
		}
		while (related_set != this);
		return true;
	}

	void end_identifier_change()
	{
		cmzn_set *related_set = this;
		do
		{
			if (related_set->temp_removed_object)
			{
				related_set->insert(related_set->temp_removed_object); // check success?
				related_set->temp_removed_object->deaccess(&related_set->temp_removed_object);
			}
			related_set = related_set->next;
		}
		while (related_set != this);
	}

	/**
	 * A specialised iterator class which wraps a reference to a container and an
	 * iterator in it, suitable for use from external API because the container
	 * cannot be destroyed before the iterator.
	 * It is also reference counted.
	 */
	class ext_iterator
	{
		cmzn_set *container;
		iterator iter;
		int access_count;

		ext_iterator(cmzn_set *container) :
			container(container->access()),
			iter(container->begin()),
			access_count(1)
		{
		}

		~ext_iterator()
		{
			// the container may be destroyed immediately before the iterator;
			// hopefully not a problem
			container->deaccess(&container);
		}

	public:

		static ext_iterator *create(cmzn_set *container)
		{
			if (container)
				return new ext_iterator(container);
			return 0;
		}

		ext_iterator *access()
		{
			++access_count;
			return this;
		}

		static int deaccess(ext_iterator* &iterator)
		{
			if (!iterator)
				return CMZN_ERROR_ARGUMENT;
			--(iterator->access_count);
			if (iterator->access_count <= 0)
				delete iterator;
			iterator = 0;
			return CMZN_OK;
		}

		Key next()
		{
			if (iter != container->end())
			{
				Key object = *iter;
				++iter;
				return object->access();
			}
			return 0;
		}

		Key next_non_access()
		{
			if (iter != container->end())
			{
				Key object = *iter;
				++iter;
				return object;
			}
			return 0;
		}

	};
};

#endif /* !defined (CMZN_SET_HPP) */
