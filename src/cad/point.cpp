/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "point.h"

Point::Point( Entity* parent )
    : Entity( parent )
{
}

Point::Point( const gp_Pnt& pnt, Entity* parent )
    : Entity( parent )
    , m_pnt( pnt )
{
}

Point::~Point()
{
}


