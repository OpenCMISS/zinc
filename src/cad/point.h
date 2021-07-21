/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMGUI_CAD_POINT_H
#define CMGUI_CAD_POINT_H

#include "entity.h"

#include <gp_Pnt.hxx>

/**
	@author user <hsorby@eggzachary>
*/
class Point : public Entity
{
public:
    Point( Entity* parent = 0 );
    Point( const gp_Pnt& pnt, Entity* parent = 0 );

    ~Point();

    GeomType geomType() const { return Pointt; }

    inline const gp_Pnt& pnt() const { return m_pnt; }
    inline void setPnt( const gp_Pnt& pnt ) { m_pnt = pnt; }
    inline Standard_Real x() const { return m_pnt.X(); }
    inline Standard_Real y() const { return m_pnt.Y(); }
    inline Standard_Real z() const { return m_pnt.Z(); }

private:
    gp_Pnt m_pnt;
};

#endif
