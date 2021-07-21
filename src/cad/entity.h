/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef ENTITY_H
#define ENTITY_H

#include <string>
#include <list>

extern "C" {
#include "user_interface/message.h"
}

/**
    @author user <hsorby@eggzachary>
*/
class Entity
{
public:
	// all known entity types
	enum GeomType
	{
		Unknown,
		Pointt,
		BoundaryLayerPoint,
		Line,
		Circle,
		Ellipse,
		Conic,
		Parabola,
		Hyperbola,
		TrimmedCurve,
		OffsetCurve,
		BSpline,
		Bezier,
		ParametricCurve,
		BoundaryLayerCurve,
		DiscreteCurve,
		Plane,
		Nurb,
		Cylinder,
		Sphere,
		Cone,
		Torus,
		RuledSurface,
		ParametricSurface,
		ProjectionFace,
		BSplineSurface,
		BezierSurface,
		ConicalSurface,
		ElementarySurface,
		LinearExtrusionSurface,
		OffsetSurface,
		GeomSurface,
		SurfaceOfRevolution,
		SweptSurface,
		RectangularTrimmedSurface,
		BoundaryLayerSurface,
		DiscreteSurface,
		CompoundSurface,
		Volume,
		DiscreteVolume
	};

	// return a string describing the entity type
	std::string geomTypeString() const
	{
		const char *name[] =
		{
			"Unknown",
			"Point",
			"Boundary layer point",
			"Line",
			"Circle",
			"Ellipse",
			"Conic",
			"Parabola",
			"Hyperbola",
			"Trimmed curve",
			"Offset curve",
			"BSpline curve",
			"Bezier curve",
			"Parametric curve",
			"Boundary layer curve",
			"Discrete curve",
			"Plane",
			"Nurb surface",
			"Cylinder",
			"Sphere",
			"Cone",
			"Toroidal surface",
			"Ruled surface",
			"Parametric surface",
			"Projection surface",
			"BSpline surface",
			"Bezier surface",
			"Conical surface",
			"Elementary surface",
			"Linear extrusion surface",
			"Offset surface",
			"Surface",
			"Surface of Revolution",
			"Swept surface",
			"Rectangular trimmed surface",
			"Boundary layer surface",
			"Discrete surface",
			"Compound surface",
			"Volume",
			"Discrete volume"
		};
		unsigned int type = (unsigned int)geomType();
		if ( type >= ( sizeof(name) / sizeof(name[0]) ) )
			return std::string( "Undefined" );
		else
			return std::string( name[type] );
	}

	virtual GeomType geomType() const { return Unknown; }
	virtual void information() const {}

	Entity( Entity* parent = 0 );
	virtual ~Entity();

private:
	Entity* m_parent;
	std::list<Entity*> m_children;

};

#endif
