/*******************************************************************************
FILE : callback_class.hpp

LAST MODIFIED : 21 February 2007

DESCRIPTION :
Template definitions for callbacks.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (CALLBACK_CLASS_HPP)
#define CALLBACK_CLASS_HPP

template < class Object > class Callback_base
/*****************************************************************************
LAST MODIFIED : 21 February 2007

DESCRIPTION :
============================================================================*/
{
public:
	int operator()(Object object)
	{
		return callback_function(object);
	}
	virtual int callback_function(Object object) = 0;

	virtual ~Callback_base()
	{
	}
};

template < class Object, class Callee, class MemberFunction > class Callback_member_callback : public Callback_base< Object >
/*****************************************************************************
LAST MODIFIED : 21 February 2007

DESCRIPTION :
============================================================================*/
{
private:
	Callee *callee;
	MemberFunction member_function;

public:
	Callback_member_callback(
		Callee *callee, MemberFunction member_function) :
		callee(callee), member_function(member_function)
	{
	}

	virtual int callback_function(Object object)
	{
		return (callee->*member_function)(object);
	}
		
	virtual ~Callback_member_callback()
	{
	}
};

#endif /* !defined (CALLBACK_CLASS_HPP) */
