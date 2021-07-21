/*******************************************************************************
FILE : graphics_object_highlight.hpp

==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef GRAPHICS_OBJECT_HIGHLIGHT_HPP
#define GRAPHICS_OBJECT_HIGHLIGHT_HPP

#include "computed_field/computed_field_subobject_group.hpp"

class SubObjectGroupHighlightFunctor
	{
	private:
		int(Computed_field_subobject_group::*function_pointer)(int);
		Computed_field_subobject_group *group;
		int contains_all;

	public:

		SubObjectGroupHighlightFunctor(Computed_field_subobject_group* group_in,
			int(Computed_field_subobject_group::*function_pointer_in)(int))
		{
			group = group_in;
			function_pointer=function_pointer_in;
			contains_all = 0;
		};

		int call(int object_name)
		{
			if (contains_all)
			{
				return 1;
			}
			else
			{
				return (*group.*function_pointer)(object_name);
			}
		};

		int setContainsAll(int flag)
		{
			return contains_all = flag;
		}

		~SubObjectGroupHighlightFunctor()
		{
		};
	};

#endif /* GRAPHICS_OBJECT_HIGHLIGHT_HPP */
