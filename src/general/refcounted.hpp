/**
 * FILE : general/refcounted.hpp
 * 
 * Base class for intrusively reference counted objects.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_GENERAL_REFCOUNTED_HPP)
#define CMZN_GENERAL_REFCOUNTED_HPP

namespace cmzn
{

/**
 * Base class for intrusively reference counted objects.
 * Constructed on heap with refCount of 1.
 */
class RefCounted
{
	template<class REFCOUNTED> friend const REFCOUNTED* Access(const REFCOUNTED* object);
	template<class REFCOUNTED> friend REFCOUNTED* Access(REFCOUNTED* object);

	template<class REFCOUNTED> friend void Deaccess(const REFCOUNTED* &object);
	template<class REFCOUNTED> friend void Deaccess(REFCOUNTED* &object);

	template<class REFCOUNTED> friend void Reaccess(const REFCOUNTED* &object, const REFCOUNTED* newObject);
	template<class REFCOUNTED> friend void Reaccess(REFCOUNTED* &object, REFCOUNTED* newObject);

protected:
	mutable int access_count;

	RefCounted() :
		access_count(1)
	{
	}

	virtual ~RefCounted()
	{
	}

	inline void access() const
	{
		++this->access_count;
	}

	void deaccess() const
	{
		--this->access_count;
		if (this->access_count <= 0)
			delete this;
	}
};

template<class REFCOUNTED> inline const REFCOUNTED* Access(const REFCOUNTED* object)
{
	if (object)
		object->access();
	return object;
}

template<class REFCOUNTED> inline REFCOUNTED* Access(REFCOUNTED* object)
{
	if (object)
		object->access();
	return object;
}

template<class REFCOUNTED> inline void Deaccess(const REFCOUNTED* &object)
{
	if (object)
	{
		object->deaccess();
		object = 0;
	}
}

template<class REFCOUNTED> inline void Deaccess(REFCOUNTED* &object)
{
	if (object)
	{
		object->deaccess();
		object = 0;
	}
}

template<class REFCOUNTED> inline void Reaccess(const REFCOUNTED* &object, const REFCOUNTED* newObject)
{
	// access first to handle object==newObject
	if (newObject)
		newObject->access();
	Deaccess(object);
	object = newObject;
}

template<class REFCOUNTED> inline void Reaccess(REFCOUNTED* &object, REFCOUNTED* newObject)
{
	// access first to handle object==newObject
	if (newObject)
		newObject->access();
	Deaccess(object);
	object = newObject;
}

}

#endif /* !defined (CMZN_GENERAL_REFCOUNTED_HPP) */
