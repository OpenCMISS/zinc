/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//
// C++ Implementation: edge
//
// Description:
//
//
// Author: user <hsorby@eggzachary>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "curve.h"

#include <BRep_Tool.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_OffsetCurve.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Parabola.hxx>
#include <Geom_Hyperbola.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Line.hxx>
#include <Geom_Conic.hxx>
#include <Geom_Surface.hxx>

#include <Geom2d_BSplineCurve.hxx>
#include <Geom2d_BezierCurve.hxx>
#include <Geom2d_OffsetCurve.hxx>
#include <Geom2d_Ellipse.hxx>
#include <Geom2d_Parabola.hxx>
#include <Geom2d_Hyperbola.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Geom2d_Circle.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_Conic.hxx>

//#include <Adaptor3d_CurveOnSurface.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <TopoDS_Vertex.hxx>
#include <TColgp_SequenceOfPnt.hxx>
#include <TColgp_SequenceOfPnt2d.hxx>
#include <ShapeAnalysis_Curve.hxx>
#include <TopExp.hxx>

//#include <TopExp_Explorer.hxx>
//#include <TopoDS.hxx>
//#include <BRep_Tool.hxx>
//#include <Geom_BezierSurface.hxx>
//#include <Geom_BezierCurve.hxx>
//#include <Handle_Geom_BezierSurface.hxx>
//#include <Geom_BoundedSurface.hxx>
//#include <Handle_Geom_BoundedSurface.hxx>
//#include <Geom_BSplineSurface.hxx>
//#include <Geom_BSplineCurve.hxx>
//#include <Handle_Geom_BSplineSurface.hxx>
//#include <Handle_Geom_ConicalSurface.hxx>
//#include <Handle_Geom_CylindricalSurface.hxx>
//#include <Handle_Geom_ElementarySurface.hxx>
//#include <Geom_OffsetCurve.hxx>
//#include <Handle_Geom_OffsetSurface.hxx>
//#include <Handle_Geom_Plane.hxx>
//#include <Geom_TrimmedCurve.hxx>
//#include <Handle_Geom_RectangularTrimmedSurface.hxx>
//#include <Handle_Geom_SphericalSurface.hxx>
//#include <Geom_Surface.hxx>
//#include <Handle_Geom_Surface.hxx>
//#include <Handle_Geom_SurfaceOfLinearExtrusion.hxx>
//#include <Handle_Geom_SurfaceOfRevolution.hxx>
//#include <Handle_Geom_SweptSurface.hxx>
//#include <Handle_Geom_ToroidalSurface.hxx>
//#include <Geom_Circle.hxx>
//#include <Geom_Ellipse.hxx>
//#include <Geom_Hyperbola.hxx>
//#include <Geom_Line.hxx>
//#include <Geom_Parabola.hxx>

extern "C" {
#include "general/debug.h"
}

Curve::Curve( Entity* parent )
    : Entity( parent )
{
}

Curve::Curve( TopoDS_Edge edge, Entity* parent )
    : Entity( parent )
    , m_edge( edge )
	, m_colour()
	, m_edgeOGLRep()
	, m_colourOGLRep()
{
	m_curve.Nullify();
	m_curve2D.Nullify();
	m_curve = BRep_Tool::Curve( m_edge, m_s0, m_s1 );
}


Curve::~Curve()
{
}

int Curve::curvePoint(double s, double *point, double *uDerivative, double *vDerivative) const
{
	USE_PARAMETER(uDerivative);
	USE_PARAMETER(vDerivative);
	int return_code = 0;
	if (point)
	{
		if (!m_curve.IsNull())
		{
			gp_Pnt pnt = m_curve->Value(s);
			point[0] = pnt.X();
			point[1] = pnt.Y();
			point[2] = pnt.Z();
			return_code = 1;
		}
		else if (!m_curve2D.IsNull())
		{
			Standard_Real u, v;
			m_curve2D->Value(s).Coord(u, v);
			Handle_Geom_Surface surf = BRep_Tool::Surface( m_face );
			gp_Pnt pnt = surf->Value( u, v );
			point[0] = pnt.X();
			point[1] = pnt.Y();
			point[2] = pnt.Z();
			return_code = 1;
		}
	}

	return return_code;
}

int Curve::sParameter(int parameter_index, double &s) const
{
	int return_code = 0;
	if ((parameter_index >= 0) && (parameter_index < (int)m_curveSRep.size()))
	{
		s = m_curveSRep[parameter_index];
		return_code = 1;
	}

	return return_code;
}

void Curve::tessellate()
{
	//bool success = false;
	if ( isInifinite() )
	{
		/** \todo output warning message? */
	}
	else if ( !m_curve.IsNull() )
	{
		// Making a 3D line
		//GCPnts_AbscissaPoint::
		GeomAdaptor_Curve c(m_curve);
		Standard_Real pixels_per_mm = 5.71;
		Standard_Real angular_deflection, curvature_deflection;
		angular_deflection = curvature_deflection = 1.0/pixels_per_mm;
		GCPnts_TangentialDeflection tangential_deflection(c, m_s0, m_s1, angular_deflection, curvature_deflection);
		Standard_Integer num_points = tangential_deflection.NbPoints();
		if (num_points > 0)
		{
			m_curveSRep.resize(num_points);
			for (Standard_Integer i = 0; i < num_points; i++)
			{
				m_curveSRep[i] = tangential_deflection.Parameter(i + 1);
			}
		}

		/*
		TColgp_SequenceOfPnt pnts;
		if ( ShapeAnalysis_Curve::GetSamplePoints( m_curve, m_s0, m_s1, pnts ) )
		{
			m_edgeOGLRep.resize( 3 * pnts.Length() );
			m_colourOGLRep.resize( 4 * pnts.Length() );
			for ( Standard_Integer i = 0; i < pnts.Length(); i++ )
			{
				m_edgeOGLRep[ 3 * i + 0 ] = pnt.X();
				m_edgeOGLRep[ 3 * i + 1 ] = pnt.Y();
				m_edgeOGLRep[ 3 * i + 2 ] = pnt.Z();

				m_colourOGLRep[ 4 * i + 0 ] = m_colour.Red();
				m_colourOGLRep[ 4 * i + 1 ] = m_colour.Green();
				m_colourOGLRep[ 4 * i + 2 ] = m_colour.Blue();
				m_colourOGLRep[ 4 * i + 3 ] = 1.0;
			}
			success = true;
		}
		else
		{
			printf( "Got zero sample points from curve\n" );
		}
		*/
	}
	else if ( !m_curve2D.IsNull() )
	{
		TColgp_SequenceOfPnt2d pnts;
		Geom2dAdaptor_Curve c(m_curve2D);
		Standard_Real pixels_per_mm = 5.71;
		Standard_Real angular_deflection, curvature_deflection;
		angular_deflection = curvature_deflection = 1.0/pixels_per_mm;
		GCPnts_TangentialDeflection tangential_deflection(c, m_s0, m_s1, angular_deflection, curvature_deflection);
		Standard_Integer num_points = tangential_deflection.NbPoints();
		m_curveSRep.resize(0);
		if (num_points > 0)
		{
			m_curveSRep.resize(num_points);
			for (Standard_Integer i = 0; i < num_points; i++)
			{
				m_curveSRep[i] = tangential_deflection.Parameter(i + 1);
			}
		}
		/*
		if ( ShapeAnalysis_Curve::GetSamplePoints( m_curve2D, m_s0, m_s1, pnts ) )
		{
			Standard_Real u, v;
			m_edgeOGLRep.resize( 3 * pnts.Length() );
			m_colourOGLRep.resize( 4 * pnts.Length() );
			for ( Standard_Integer i = 0; i < pnts.Length(); i++ )
			{
				pnts.Value( i + 1 ).Coord( u, v );
				Handle_Geom_Surface surf = BRep_Tool::Surface( m_face );
				gp_Pnt pnt = surf->Value( u, v );
				m_edgeOGLRep[ 3 * i + 0 ] = pnt.X();
				m_edgeOGLRep[ 3 * i + 1 ] = pnt.Y();
				m_edgeOGLRep[ 3 * i + 2 ] = pnt.Z();

				m_colourOGLRep[ 4 * i + 0 ] = m_colour.Red();
				m_colourOGLRep[ 4 * i + 1 ] = m_colour.Green();
				m_colourOGLRep[ 4 * i + 2 ] = m_colour.Blue();
				m_colourOGLRep[ 4 * i + 3 ] = 1.0;
			}
			success = true;
		}
		else
		{
			printf( "Got zero sample points from 2D curve\n" );
		}
		*/
	}

	//return success;
}

bool Curve::buildOpenGLRep()
{
	bool success = false;
	if ( isInifinite() )
	{
		/** \todo output warning message? */
	}
	else if ( !m_curve.IsNull() )
	{
		// Making a 3D line
		TColgp_SequenceOfPnt pnts;
		if ( ShapeAnalysis_Curve::GetSamplePoints( m_curve, m_s0, m_s1, pnts ) )
		{
			if ( pnts.Length() == 0 )
				printf( " !!!!!!!!!!!!! zero points in curve !!!!!!!!!!!!!!!!!\n" );
			m_edgeOGLRep.resize( 3 * pnts.Length() );
			m_colourOGLRep.resize( 4 * pnts.Length() );
			for ( int i = 0; i < pnts.Length(); i++ )
			{
				gp_Pnt pnt = pnts.Value( i + 1 );
				m_edgeOGLRep[ 3 * i + 0 ] = static_cast<float>(pnt.X());
				m_edgeOGLRep[ 3 * i + 1 ] = static_cast<float>(pnt.Y());
				m_edgeOGLRep[ 3 * i + 2 ] = static_cast<float>(pnt.Z());

				m_colourOGLRep[ 4 * i + 0 ] = static_cast<float>(m_colour.Red());
				m_colourOGLRep[ 4 * i + 1 ] = static_cast<float>(m_colour.Green());
				m_colourOGLRep[ 4 * i + 2 ] = static_cast<float>(m_colour.Blue());
				m_colourOGLRep[ 4 * i + 3 ] = 1.0;
			}
			success = true;
		}
	}
	else if ( !m_curve2D.IsNull() )
	{
		TColgp_SequenceOfPnt2d pnts;
		if ( ShapeAnalysis_Curve::GetSamplePoints( m_curve2D, m_s0, m_s1, pnts ) )
		{
			if ( pnts.Length() == 0 )
				printf( " !!!!!!!!!!!!! zero points in curve !!!!!!!!!!!!!!!!!\n" );
			Standard_Real u, v;
			m_edgeOGLRep.resize( 3 * pnts.Length() );
			m_colourOGLRep.resize( 4 * pnts.Length() );
			for ( int i = 0; i < pnts.Length(); i++ )
			{
				pnts.Value( i + 1 ).Coord( u, v );
				Handle_Geom_Surface surf = BRep_Tool::Surface( m_face );
				gp_Pnt pnt = surf->Value( u, v );
				m_edgeOGLRep[ 3 * i + 0 ] = static_cast<float>(pnt.X());
				m_edgeOGLRep[ 3 * i + 1 ] = static_cast<float>(pnt.Y());
				m_edgeOGLRep[ 3 * i + 2 ] = static_cast<float>(pnt.Z());

				m_colourOGLRep[ 4 * i + 0 ] = static_cast<float>(m_colour.Red());
				m_colourOGLRep[ 4 * i + 1 ] = static_cast<float>(m_colour.Green());
				m_colourOGLRep[ 4 * i + 2 ] = static_cast<float>(m_colour.Blue());
				m_colourOGLRep[ 4 * i + 3 ] = 1.0;
			}
			if ( &m_edgeOGLRep[0] == 0 )
				printf( "dead ogl rep %d\n", pnts.Length() );
			success = true;
		}
		else
		{
			printf( "Got zero sample points\n" );
		}
	}

	return success;
}

void Curve::setTrimmed( TopoDS_Face face )
{
    m_curve2D = BRep_Tool::CurveOnSurface( m_edge, face, m_s0, m_s1);
    if ( m_curve2D.IsNull() )
    {
      /** \todo output warning here */
    }
    else
    {
        m_face = face;
    }
}

Entity::GeomType Curve::geomType() const
{
    if ( !m_curve.IsNull() )
    {
        if (m_curve->DynamicType() == STANDARD_TYPE(Geom_Circle))
            return Circle;
        else if (m_curve->DynamicType() == STANDARD_TYPE(Geom_Line))
            return Line;
        else if (m_curve->DynamicType() == STANDARD_TYPE(Geom_Parabola))
            return Parabola;
        else if (m_curve->DynamicType() == STANDARD_TYPE(Geom_Hyperbola))
            return Hyperbola;
        else if (m_curve->DynamicType() == STANDARD_TYPE(Geom_TrimmedCurve))
            return TrimmedCurve;
        else if (m_curve->DynamicType() == STANDARD_TYPE(Geom_OffsetCurve))
            return OffsetCurve;
        else if (m_curve->DynamicType() == STANDARD_TYPE(Geom_Ellipse))
            return Ellipse;
        else if (m_curve->DynamicType() == STANDARD_TYPE(Geom_BSplineCurve))
            return BSpline;
        else if (m_curve->DynamicType() == STANDARD_TYPE(Geom_BezierCurve))
            return Bezier;
        else if (m_curve->DynamicType() == STANDARD_TYPE(Geom_Conic))
            return Conic;
    }
    else if ( !m_curve2D.IsNull() )
    {
        if (m_curve2D->DynamicType() == STANDARD_TYPE(Geom2d_Circle))
            return Circle;
        else if (m_curve2D->DynamicType() == STANDARD_TYPE(Geom2d_Line))
            return Line;
        else if (m_curve2D->DynamicType() == STANDARD_TYPE(Geom2d_Ellipse))
            return Ellipse;
        else if (m_curve2D->DynamicType() == STANDARD_TYPE(Geom2d_Parabola))
            return Parabola;
        else if (m_curve2D->DynamicType() == STANDARD_TYPE(Geom2d_Hyperbola))
            return Hyperbola;
        else if (m_curve2D->DynamicType() == STANDARD_TYPE(Geom2d_TrimmedCurve))
            return TrimmedCurve;
        else if (m_curve2D->DynamicType() == STANDARD_TYPE(Geom2d_OffsetCurve))
            return OffsetCurve;
        else if (m_curve2D->DynamicType() == STANDARD_TYPE(Geom2d_BSplineCurve))
            return BSpline;
        else if (m_curve2D->DynamicType() == STANDARD_TYPE(Geom2d_BezierCurve))
            return Bezier;
        else if (m_curve2D->DynamicType() == STANDARD_TYPE(Geom2d_Conic))
            return Conic;
    }

    return Unknown;
}

void Curve::information() const
{
	display_message(INFORMATION_MESSAGE,
		"  ");
	if (m_curve.IsNull() && !m_curve2D.IsNull())
		display_message(INFORMATION_MESSAGE, "2D ");
	display_message(INFORMATION_MESSAGE, "%s\n", geomTypeString().c_str());
}

