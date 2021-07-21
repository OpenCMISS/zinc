/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef TRANSFORMOCCMODEL_H
#define TRANSFORMOCCMODEL_H

#include <TopoDS_Shape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <Handle_TopTools_HSequenceOfShape.hxx>
#include <Handle_TDocStd_Document.hxx>
#include <Handle_XCAFDoc_ShapeTool.hxx>
#include <Handle_XCAFDoc_ColorTool.hxx>
#include <Quantity_Color.hxx>
#include <TDF_Label.hxx>

#include <vector>
#include <list>

#include "entity.h"
#include "point.h"
#include "curve.h"
#include "surface.h"
#include "geometricshape.h"
#include "topologicalshape.h"
#include "topologicalshaperoot.h"

/**
	@author user <hsorby@eggzachary>
*/
class TransformOCCModel
{
public:
	TransformOCCModel();
	~TransformOCCModel();

	void mapShapes( Handle_TopTools_HSequenceOfShape shapes );
	void mapShape( TopoDS_Shape shape );

	void mapShapes( Handle_TDocStd_Document doc );

	void buildGeometricShapes();
	const std::vector<GeometricShape*>& geometricShapes() const { return m_geometricShapes; }

	int pointCount() const { return m_points.size(); }
	int curveCount() const { return m_curves.size(); }
	int surfaceCount() const { return m_surfaces.size(); }
	
	const Point* point( int index ) const { return m_points.at( index ); }
	const Curve* curve( int index ) const { return m_curves.at( index ); }
	const Surface* surface( int index ) const { return m_surfaces.at( index ); }

private:
	void healGeometry( TopoDS_Shape shape, double tolerance, bool fixsmalledges,
						bool fixspotstripfaces, bool sewfaces,
						bool makesolids = false);
	void clearMaps();
	void recurseTopologicalShapesAndBuildGeometricShapes( TopologicalShape* shape );
	void buildMaps( const TopoDS_Shape& shape );
	void createShapeColourStacks( const TDF_Label& label );
	bool hasChildren( const TDF_Label& label );
	void addLabelContents( const TDF_Label& label );
	bool mapStacks();
	void extractPoints( GeometricShape* geoShape );
	void extractCurvesAndSurfaces( GeometricShape* geoShape );
	void extractShapeInfo();
	void basicTraversal();
	bool stylePrint( const TDF_Label& label, const TopLoc_Location& aLocation, TopologicalShape* shape );
	void labelTraversal(const TDF_Label& aLabel, const TopLoc_Location& aLocation, TopologicalShape* parent );

	void printLabelInfo( const TDF_Label& label, int depth );
	void plumbLabel( const TDF_Label& label,
		const Handle_XCAFDoc_ShapeTool& shapeTool, const Handle_XCAFDoc_ColorTool& colourTool );
	Standard_Boolean extractColour( const TDF_Label& label, Quantity_Color &colour );	
	Standard_Boolean extractColour( const TopoDS_Shape& shape, const Handle_XCAFDoc_ColorTool& colourTool, Quantity_Color &colour );	
	Standard_Boolean extractColour( const TDF_Label& label, const Handle_XCAFDoc_ColorTool& colourTool, Quantity_Color &colour );	
	void traverseShape( const TopoDS_Shape& shape );
	void traverseLabel( const TDF_Label& label );

private:
	std::vector<Point*> m_points;
	std::vector<Curve*> m_curves;
	std::vector<Surface*> m_surfaces;

	std::vector<GeometricShape*> m_geometricShapes;

	TopologicalShapeRoot m_rootShape;

	TopTools_IndexedMapOfShape m_cmap, m_csmap, m_somap, m_shmap, m_fmap, m_emap, m_vmap;
	Handle_TDocStd_Document m_doc;

	std::vector<TopoDS_Shape> m_shapeStack;
	std::vector<Quantity_Color> m_colourStack;
	Quantity_Color m_currentColour;
};

#endif
