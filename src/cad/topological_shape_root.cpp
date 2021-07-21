/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "topologicalshaperoot.h"

TopologicalShapeRoot::TopologicalShapeRoot()
{
}

TopologicalShapeRoot::~TopologicalShapeRoot()
{
	while ( m_children.size() > 0 )
	{
		delete m_children.back();
		m_children.pop_back();
	}
}

