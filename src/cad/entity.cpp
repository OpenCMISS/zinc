/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "entity.h"

Entity::Entity( Entity* parent )
    : m_parent( parent )
    , m_children()
{
}


Entity::~Entity()
{
}


