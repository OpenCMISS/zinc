/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef OCCPARTFACTORY_H
#define OCCPARTFACTORY_H

#include <TopoDS_Shape.hxx>
// #include <Handle_AIS_Shape.hxx>

/**
	@author user <hsorby@eggzachary>
*/
class OCCPartFactory
{
public:
    OCCPartFactory();

    ~OCCPartFactory();

    static TopoDS_Shape makeBottle( const Standard_Real width,
                                    const Standard_Real height,
                                    const Standard_Real depth);
    
    static TopoDS_Shape makeCube( const Standard_Real width,
                                  const Standard_Real height,
                                  const Standard_Real depth);
    
    static TopoDS_Shape makeCylinder( const Standard_Real radius,
                                      const Standard_Real height);

};

#endif
