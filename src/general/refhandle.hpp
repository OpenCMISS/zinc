/**
 * FILE : general/refhandle.hpp
 * 
 * Simple intrusive smart pointer template for classes derived from
 * cmzn::RefCounted, supporting access/deaccess methods.
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (CMZN_GENERAL_REFHANDLE_HPP)
#define CMZN_GENERAL_REFHANDLE_HPP

#include "general/refcounted.hpp"

namespace cmzn
{

template <typename T> class RefHandle
{
	template<typename POINTEE>
		friend POINTEE* GetImpl(const RefHandle<POINTEE>& refHandle);
	template<typename POINTEE>
		friend void SetImpl(RefHandle<POINTEE>& refHandle, POINTEE* newObject);
	template<typename POINTEE>
		friend bool operator==(const RefHandle<POINTEE>& refHandle1, const RefHandle<POINTEE>& refHandle2);
	template<typename POINTEE>
		friend bool operator!=(const RefHandle<POINTEE>& refHandle1, const RefHandle<POINTEE>& refHandle2);

private:
	T* object;

public:
	RefHandle() :
		object(0)
	{
	}

	// take ownership of reference; assumes created with access count of 1
	explicit RefHandle(T* objectIn) :
		object(objectIn)
	{
	}

	RefHandle(const RefHandle<T>& refHandleIn) :
		object(cmzn::Access(refHandleIn.object))
	{
	}

	~RefHandle()
	{
		cmzn::Deaccess(this->object);
	}

	RefHandle<T>& operator=(const RefHandle<T>& refHandleIn)
	{
		cmzn::Reaccess(this->object, refHandleIn.object);
		return *this;
	}

	T& operator*()
	{
		return *this->object;
	}

	T* operator->()
	{
		return this->object;
	}

	const T* operator->() const
	{
		return this->object;
	}

	operator bool() const
	{
		return (this->object != 0);
	}
};

template<typename POINTEE> inline POINTEE* GetImpl(const RefHandle<POINTEE>& refHandle)
{
	return refHandle.object;
}

// take ownership of reference in newImpl
template<typename POINTEE> inline void SetImpl(RefHandle<POINTEE>& refHandle, POINTEE* newObject)
{
	Deaccess(refHandle.object);
	refHandle.object = newObject;
}

template<typename POINTEE> inline bool operator==(
	const RefHandle<POINTEE>& refHandle1, const RefHandle<POINTEE>& refHandle2)
{
	return refHandle1.object == refHandle2.object;
}

template<typename POINTEE> inline bool operator!=(
	const RefHandle<POINTEE>& refHandle1, const RefHandle<POINTEE>& refHandle2)
{
	return refHandle1.object != refHandle2.object;
}

}

#endif /* !defined (CMZN_GENERAL_REFHANDLE_HPP) */
