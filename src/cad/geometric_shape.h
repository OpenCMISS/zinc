/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined CMGUI_CAD_GEOMETRICSHAPE_H
#define CMGUI_CAD_GEOMETRICSHAPE_H

#include <vector>

#include <Quantity_Color.hxx>

extern "C" {
#include "api/cmiss_field_cad.h"
}

#include "point.h"
#include "curve.h"
#include "surface.h"

typedef std::vector<int>::iterator Surface_iterator;
typedef std::vector<int>::const_iterator Surface_const_iterator;

class GeometricShape
{
	public:
		GeometricShape();
		~GeometricShape();

		void surfaceColour( const Quantity_Color& c ) { m_surfaceColour = c; }
		void curveColour( const Quantity_Color& c ) { m_curveColour = c; }

		const Quantity_Color& surfaceColour() const { return m_surfaceColour; }
		const Quantity_Color& curveColour() const { return m_curveColour; }

		int pointCount() const { return m_points.size(); }
		int curveCount() const { return m_curves.size(); }
		int surfaceCount() const { return m_surfaces.size(); }

		const Point* point( int index ) const { return m_points.at( index ); }
		const Curve* curve( int index ) const { return m_curves.at( index ); }
		const Surface* surface( int index ) const { return m_surfaces.at( index ); }

		Surface_iterator surface_index_iterator() { return m_surfaceIndexes.begin(); }
		Surface_const_iterator surface_index_iterator() const { return m_surfaceIndexes.begin(); }

		void append( Point* pt ) { m_points.push_back( pt ); }
		void append( Curve* c ) { m_curves.push_back( c ); }
		void append( Surface* s ) { m_surfaceIndexes.push_back(m_surfaces.size()); m_surfaces.push_back( s ); }

	private:
		std::vector<Point*> m_points;
		std::vector<Curve*> m_curves;
		std::vector<Surface*> m_surfaces;
		std::vector<cmzn_cad_surface_identifier> m_surfaceIndexes;

		Quantity_Color m_surfaceColour;
		Quantity_Color m_curveColour;
};


#endif
