/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "surface.h"

#include <Poly_Triangulation.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColgp_Array1OfDir.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <Poly_Triangle.hxx>
#include <BRep_Tool.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepMesh_FastDiscret.hxx>
#include <BRepGProp_Face.hxx>
#include <StdPrs_ToolShadedShape.hxx>
#include <Poly_Connect.hxx>
#include <TColgp_HArray1OfVec.hxx>
#include <BRepTools.hxx>
#include <BRepMesh.hxx>
#include <Precision.hxx>
#include <GeomLProp_SLProps.hxx>

#include <Geom_ConicalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_ElementarySurface.hxx>
#include <Geom_OffsetSurface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_SurfaceOfLinearExtrusion.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <Geom_SweptSurface.hxx>
#include <Geom_ToroidalSurface.hxx>

Surface::Surface( Entity* parent )
    : Entity( parent )
{
}

Surface::Surface( TopoDS_Face face, Entity* parent )
    : Entity( parent )
    , m_face( face )
	, m_surface( BRep_Tool::Surface( face ) )
{
}

Surface::~Surface()
{
}

Standard_Boolean Surface::triangleIsValid(const gp_Pnt& P1, const gp_Pnt& P2, const gp_Pnt& P3)
{
	gp_Vec V1(P1,P2);               // V1=(P1,P2)
	gp_Vec V2(P2,P3);               // V2=(P2,P3)
	gp_Vec V3(P3,P1);               // V3=(P3,P1)

	if ((V1.SquareMagnitude() > 1.e-10) && (V2.SquareMagnitude() > 1.e-10) && (V3.SquareMagnitude() > 1.e-10))
	{
		gp_Vec normal = V1.Crossed(V2) + V2.Crossed(V3) + V3.Crossed(V1);
		if (normal.SquareMagnitude() > 1.e-10)
			return Standard_True;
		else
			return Standard_False;
	}

	return Standard_False;
}

void Surface::tessellate()
{
	TopLoc_Location loc;
	Handle_Poly_Triangulation triangulation = BRep_Tool::Triangulation( m_face, loc );
	if ( triangulation.IsNull() )
	{
		return;
	}
	int num_triangles = triangulation->NbTriangles();

	const TColgp_Array1OfPnt& nodes = triangulation->Nodes();
	const Poly_Array1OfTriangle& triangles = triangulation->Triangles();
	const TColgp_Array1OfPnt2d& uvNodes = triangulation->UVNodes();
	m_surfaceUVRep.resize( 2 * 3 * num_triangles );
	m_surfacePointIndecies.resize( 3 * num_triangles );
	Standard_Integer n1, n2, n3 = 0;
	Standard_Integer num_invalid_triangles = 0;
	Standard_Integer current_triangle = 0;
	for ( Standard_Integer i = 0; i < num_triangles; i++ )
	{
		Poly_Triangle triangle = triangles( i + 1 );
		bool face_reversed = (m_face.Orientation() == TopAbs_REVERSED);

		if ( face_reversed )
			triangle.Get( n1, n3, n2 );
		else
			triangle.Get( n1, n2, n3 );

		if (triangleIsValid(nodes(n1), nodes(n2), nodes(n3)))
		{
			gp_Pnt2d uv1 = uvNodes.Value(n1);
			gp_Pnt2d uv2 = uvNodes.Value(n2);
			gp_Pnt2d uv3 = uvNodes.Value(n3);

			m_surfacePointIndecies.push_back( 3 * current_triangle + 0 );
			m_surfaceUVRep[ 6 * current_triangle + 0 ] = uv1.X();
			m_surfaceUVRep[ 6 * current_triangle + 1 ] = uv1.Y();
			m_surfacePointIndecies.push_back( 3 * current_triangle + 1 );
			m_surfaceUVRep[ 6 * current_triangle + 2 ] = uv2.X();
			m_surfaceUVRep[ 6 * current_triangle + 3 ] = uv2.Y();
			m_surfacePointIndecies.push_back( 3 * current_triangle + 2 );
			m_surfaceUVRep[ 6 * current_triangle + 4 ] = uv3.X();
			m_surfaceUVRep[ 6 * current_triangle + 5 ] = uv3.Y();

			current_triangle++;
		}
		else
		{
			num_invalid_triangles++;
		}
	}
	if (num_invalid_triangles > 0)
	{
		m_surfaceUVRep.resize(2 * 3 * (num_triangles - num_invalid_triangles));
	}
}

int Surface::surfacePoint(double u, double v, double *point, double *uDerivative, double *vDerivative) const
{
	int return_code = 0;
	gp_Vec d1u, d1v;
	gp_Pnt pnt;
	if (point)
	{
		m_surface->D1(u, v, pnt, d1u, d1v );
		int reverse = 1;
		if ((m_face.Orientation() == TopAbs_REVERSED))
			reverse = -1;

		point[0] = pnt.X();
		point[1] = pnt.Y();
		point[2] = pnt.Z();
		if (uDerivative && vDerivative)
		{
			uDerivative[0] = reverse * d1u.X();
			uDerivative[1] = reverse * d1u.Y();
			uDerivative[2] = reverse * d1u.Z();
			vDerivative[0] = d1v.X();
			vDerivative[1] = d1v.Y();
			vDerivative[2] = d1v.Z();
		}
		return_code = 1;
	}

	return return_code;
}

int Surface::uvPoints(int point_index, double &u, double &v) const
{
	int return_code = 1;

	u = m_surfaceUVRep.at(2 * point_index + 0);
	v = m_surfaceUVRep.at(2 * point_index + 1);

	return return_code;
}

Entity::GeomType Surface::geomType() const
{
	if (m_surface->DynamicType() == STANDARD_TYPE(Geom_BezierSurface))
	{
		return BezierSurface;
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_BoundedSurface))
	{
		return BoundaryLayerSurface;
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_BSplineSurface))
	{
		return BSplineSurface;
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_ConicalSurface))
	{
		return ConicalSurface;
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_CylindricalSurface))
	{
		return Cylinder;
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_ElementarySurface))
	{
		return ElementarySurface;
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_OffsetSurface))
	{
		return OffsetSurface;
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_Plane))
	{
		return Plane;
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_RectangularTrimmedSurface))
	{
		return RectangularTrimmedSurface;
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_SphericalSurface))
	{
		return Sphere;
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_Surface))
	{
		return GeomSurface;
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_SurfaceOfLinearExtrusion))
	{
		return LinearExtrusionSurface;
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_SurfaceOfRevolution))
	{
		return SurfaceOfRevolution;
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_SweptSurface))
	{
		return SweptSurface;
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_ToroidalSurface))
	{
		return Torus;
	}

	return Unknown;
}

void Surface::information() const
{
	display_message(INFORMATION_MESSAGE,
		"  %s", geomTypeString().c_str());
	if (m_surface->DynamicType() == STANDARD_TYPE(Geom_CylindricalSurface))
	{
		Handle_Geom_CylindricalSurface cylinder = Handle_Geom_CylindricalSurface::DownCast(m_surface);
		display_message(INFORMATION_MESSAGE, " with radius = %.3g", cylinder->Radius());
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_Plane))
	{
		Handle_Geom_Plane plane = Handle_Geom_Plane::DownCast(m_surface);
		Standard_Real a, b, c, d;
		plane->Coefficients(a, b, c, d);
		display_message(INFORMATION_MESSAGE, " with equation: %.3g x + %.3g y + %.3g z + %.3g = 0", a, b, c, d);
	}
	else if (m_surface->DynamicType() == STANDARD_TYPE(Geom_SphericalSurface))
	{
		Handle_Geom_SphericalSurface sphere = Handle_Geom_SphericalSurface::DownCast(m_surface);
		display_message(INFORMATION_MESSAGE, " with area: %.3g", sphere->Area());
	}
	display_message(INFORMATION_MESSAGE, "\n");
}

