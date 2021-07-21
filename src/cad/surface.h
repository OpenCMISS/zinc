/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef SURFACE_H
#define SURFACE_H

#include "entity.h"

#include <vector>

#include <TopoDS_Face.hxx>
#include <Geom_Surface.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Poly_Triangle.hxx>
#include <Quantity_Color.hxx>
#include <Handle_Poly_Triangulation.hxx>
#include <Handle_TColgp_HArray1OfVec.hxx>
#include <TColgp_Array1OfDir.hxx>
/**
	@author user <hsorby@eggzachary>
*/
class Surface;
typedef const Surface *surface_id;
typedef std::vector<int>::iterator Surface_point_iterator;
typedef std::vector<int>::const_iterator Surface_point_const_iterator;

class Surface : public Entity
{
public:
	Surface( Entity* parent = 0 );
	Surface( TopoDS_Face face, Entity* parent = 0 );
	~Surface();

	surface_id id() const {return this;}
	int uvPoints(int point_index, double &u, double &v) const;
	int surfacePoint(double u, double v, double *point, double *uDerivative = 0, double *vDerivative = 0) const;

	inline bool isEmpty() const { return m_surfaceUVRep.empty(); }
	inline int pointCount() const { return m_surfaceUVRep.size() / 2; }

	Surface_point_iterator point_index_iterator() { return m_surfacePointIndecies.begin(); }
	Surface_point_const_iterator point_index_iterator() const { return m_surfacePointIndecies.begin(); }

	void tessellate();

	Entity::GeomType geomType() const;
	void information() const;

private:
	Standard_Boolean triangleIsValid(const gp_Pnt& P1, const gp_Pnt& P2, const gp_Pnt& P3);

private:
	TopoDS_Face m_face;
	Handle_Geom_Surface m_surface;
	GeomAPI_ProjectPointOnSurf m_project;
	Quantity_Color m_colour;

	std::vector<double> m_surfaceUVRep;
	std::vector<int> m_surfacePointIndecies;
};

#endif
