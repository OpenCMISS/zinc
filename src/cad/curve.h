/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//
// C++ Interface: edge
//
// Description:
//
//
// Author: user <hsorby@eggzachary>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CMGUI_CAD_CURVE_H
#define CMGUI_CAD_CURVE_H

#include "entity.h"

#include <vector>

// #include <QVector>

#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <Geom_Curve.hxx>
#include <Geom2d_Curve.hxx>
#include <Quantity_Color.hxx>

/**
	@author user <hsorby@eggzachary>
*/
class Curve : public Entity
{
public:
	Curve( Entity* parent = 0 );
	Curve( TopoDS_Edge edge, Entity* parent = 0 );
	~Curve();

	const float* points() const { return &m_edgeOGLRep[0]; }
	const float* colours() const { return &m_colourOGLRep[0]; }
	inline bool isEmpty() const { return m_curveSRep.empty(); }
	inline int pointCount() const { return m_curveSRep.size(); }
	inline void setColour( const Quantity_Color& colour ) { m_colour = colour; }

	int sParameter(int parameter_index, double &s) const;
	int curvePoint(double s, double *point, double *uDerivative = 0, double *vDerivative = 0) const;

	inline Standard_Boolean is3D() const { return !m_curve.IsNull(); }
	inline Standard_Boolean is2D() const { return !m_curve2D.IsNull(); }
	inline Standard_Boolean isClosed() const { return m_edge.Closed(); }
	inline Standard_Boolean isInifinite() const { return m_edge.Infinite(); }
	inline TopAbs_Orientation orientation() const { return m_edge.Orientation(); }
	inline Standard_Boolean isPeriodic() const
	{
		if ( !m_curve.IsNull() )
			return m_curve->IsPeriodic();
		if ( !m_curve2D.IsNull() )
			return m_curve2D->IsPeriodic();
		return false;
	}

	void setTrimmed( TopoDS_Face face );
	GeomType geomType() const;// { return Unknown; }
	bool buildOpenGLRep();
	void tessellate();

	void information() const;

private:
	TopoDS_Face m_face;
	TopoDS_Edge m_edge;
	Handle_Geom_Curve m_curve;
	Handle_Geom2d_Curve m_curve2D;
	Standard_Real m_s0, m_s1;
	Quantity_Color m_colour;

	std::vector<float> m_edgeOGLRep;
	std::vector<float> m_colourOGLRep;

	std::vector<double> m_curveSRep;
};

#endif
