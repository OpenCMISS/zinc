/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined CMGUI_CAD_TOPOLOGICALSHAPEROOT_H
#define CMGUI_CAD_TOPOLOGICALSHAPEROOT_H

#include <list>

#include "topologicalshape.h"

class TopologicalShapeRoot
{
	public:
		TopologicalShapeRoot();
		~TopologicalShapeRoot();

		void child( TopologicalShape* s ) { m_children.push_back( s ); }
		TopologicalShape* lastChild() const { return m_children.back(); }
		std::list<TopologicalShape*> children() const { return m_children; }

	private:
		std::list<TopologicalShape*> m_children;
};


#endif
