/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "geometricshape.h"

GeometricShape::GeometricShape()
{
}

GeometricShape::~GeometricShape()
{
	while ( m_points.size() > 0 )
	{
		delete m_points.back();
		m_points.pop_back();
	}

	while ( m_curves.size() > 0 )
	{
		delete m_curves.back();
		m_curves.pop_back();
	}

	while ( m_surfaces.size() > 0 )
	{
		delete m_surfaces.back();
		m_surfaces.pop_back();
	}
}

