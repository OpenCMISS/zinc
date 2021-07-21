/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "transformoccmodel.h"

#include <time.h>

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <Standard_ConstructionError.hxx>
#include <gp_Pnt.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeBuild_ReShape.hxx>
#include <ShapeFix_Shape.hxx>
#include <BRepLib.hxx>
#include <ShapeFix_Wireframe.hxx>
#include <ShapeFix_FixSmallFace.hxx>
#include <BRepOffsetAPI_Sewing.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <Geom_Surface.hxx>
#include <ShapeAnalysis.hxx>
#include <Geom_Curve.hxx>
#include <TColgp_SequenceOfPnt.hxx>
#include <ShapeAnalysis_Curve.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <TDocStd_Document.hxx>
#include <TDataStd_Name.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_LayerTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <TDF_LabelSequence.hxx>
#include <Quantity_Color.hxx>
#include <TDF_Tool.hxx>
#include <TDF_ChildIterator.hxx>
#include <XCAFPrs.hxx>
#include <XCAFPrs_Style.hxx>
#include <XCAFPrs_DataMapOfShapeStyle.hxx>
#include <XCAFPrs_DataMapOfStyleShape.hxx>
#include <XCAFPrs_DataMapIteratorOfDataMapOfStyleShape.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

#include <general/debug.h>
extern "C" {
#include "user_interface/message.h"
}

TransformOCCModel::TransformOCCModel()
{
	m_doc.Nullify();
}


TransformOCCModel::~TransformOCCModel()
{
	printf( "TransformOCCModel::~TransformOCCModel()\n" );
	for ( unsigned int i = 0; i < m_points.size(); i++ )
	{
		delete m_points[i];
	}
	for ( unsigned int i = 0; i < m_curves.size(); i++ )
	{
		delete m_curves[i];
	}
	for ( unsigned int i = 0; i < m_surfaces.size(); i++ )
	{
		delete m_surfaces[i];
	}
}

void printTDFLabel( const TDF_Label& label )
{
	Handle_XCAFDoc_ColorTool colourTool = XCAFDoc_DocumentTool::ColorTool( label );
	Handle_TDataStd_Name N;
	if ( label.FindAttribute( TDataStd_Name::GetID(), N ) )
	{
		char *string = (char *) malloc( ( N->Get().LengthOfCString() + 1 ) * sizeof( char ) );
		N->Get().ToUTF8CString( string );
		printf( "name[%d]: %s", N->Get().LengthOfCString(), string );
		free( string );
	}
	TCollection_AsciiString entry;
	TDF_Tool::Entry( label, entry );
	std::cout << ", " << entry;
	TDF_Label colourLabel;
	if( colourTool->GetColor( label, XCAFDoc_ColorGen, colourLabel) ||
		colourTool->GetColor( label, XCAFDoc_ColorSurf, colourLabel) ||
		colourTool->GetColor( label, XCAFDoc_ColorCurv, colourLabel) )
	{
		Quantity_Color colour;
		if ( colourTool->GetColor( colourLabel, colour ) )
		{
			std::cout << ", " << Quantity_Color::StringName( colour.Name() );
		}
	}
	std::cout << std::endl;
}

Standard_Boolean TransformOCCModel::extractColour( const TDF_Label& label, const Handle_XCAFDoc_ColorTool& colourTool, Quantity_Color &colour )
{
	//Quantity_Color colour = Quantity_Color();
	Standard_Boolean okCurve = false;
	Standard_Boolean okSurface = false;
	Standard_Boolean okGeneral = false;

	okCurve = colourTool->GetColor( label, XCAFDoc_ColorCurv, colour);
	if ( !okCurve )
	{
		okSurface = colourTool->GetColor( label, XCAFDoc_ColorSurf, colour);
		if ( !okSurface )
		{
			okGeneral = colourTool->GetColor( label, XCAFDoc_ColorGen, colour);
		}
	}

	if ( okCurve && okSurface )
	{
		printf( "Curve has a good colour %s\n", Quantity_Color::StringName( colour.Name() ) );
		printf( "Surface has a good colour %s\n", Quantity_Color::StringName( colour.Name() ) );
		printf( "General has a good colour %s\n", Quantity_Color::StringName( colour.Name() ) );
	}

	return ( okCurve || okSurface || okGeneral );
}

Standard_Boolean TransformOCCModel::extractColour( const TDF_Label &label, Quantity_Color &colour )
{
	USE_PARAMETER(label);
	USE_PARAMETER(colour);
	return false;
}

Standard_Boolean TransformOCCModel::extractColour( const TopoDS_Shape& shape, const Handle_XCAFDoc_ColorTool& colourTool, Quantity_Color &colour )
{
	//Quantity_Color colour = Quantity_Color();
	Standard_Boolean ok = false;

	ok = colourTool->GetColor( shape, XCAFDoc_ColorCurv, colour);
	if ( ok )
	{
		//ok = colourTool->GetColor( shape, XCAFDoc_ColorSurf, colour);
		printf( "Curve has a good colour %s\n", Quantity_Color::StringName( colour.Name() ) );
		//return ok;
	}

	ok = colourTool->GetColor( shape, XCAFDoc_ColorSurf, colour);
	if ( ok )
	{
		printf( "Surface has a good colour %s\n", Quantity_Color::StringName( colour.Name() ) );
		//return ok;
	}

	ok = colourTool->GetColor( shape, XCAFDoc_ColorGen, colour);
	{
		printf( "General has a good colour %s\n", Quantity_Color::StringName( colour.Name() ) );
	}

	return ok;
}

void TransformOCCModel::traverseShape( const TopoDS_Shape& shape )
{
	TopoDS_Iterator it( shape );
	for ( ; it.More(); it.Next() )
	{
		const TopoDS_Shape& child = it.Value();
		if ( child.ShapeType() < TopAbs_EDGE )
			traverseShape( child );

		mapShape( child );
	}
}

bool TransformOCCModel::hasChildren( const TDF_Label& label )
{
	TDF_ChildIterator it( label );

	return (it.More() == (Standard_Boolean)1);
}

void TransformOCCModel::addLabelContents( const TDF_Label& label )
{
	Quantity_Color colour;
	Handle_XCAFDoc_ColorTool colourTool = XCAFDoc_DocumentTool::ColorTool( m_doc->Main() );

	TopoDS_Shape shape = XCAFDoc_ShapeTool::GetShape( label );
	//if ( !shape.IsNull() )
	//	std::cout << "shape from label (" << shape.ShapeType() << ") " << shape.HashCode( 100 ) << std::endl;
	if ( extractColour( label, colourTool, colour ) )
	{
		printf( "adding shape direct\n" );
		printf( "adding colour direct: %s\n", Quantity_Color::StringName( colour.Name() ) );
		m_shapeStack.push_back( shape );
		m_colourStack.push_back( colour );
	}
	else
	{
		if ( XCAFDoc_ShapeTool::IsReference( label ) )
		{
			TDF_Label ref;
			XCAFDoc_ShapeTool::GetReferredShape( label, ref );
			if ( XCAFDoc_ShapeTool::IsAssembly( ref ) )
			{
				int stackIndex = m_shapeStack.size();
				createShapeColourStacks( ref );
				// Move all recently added shapes to label location
				for ( unsigned int i = stackIndex; i < m_shapeStack.size(); i++ )
				{
					m_shapeStack[i].Move( XCAFDoc_ShapeTool::GetLocation( label ) );
				}
			}
			else
			{
				if ( extractColour( ref, colourTool, colour ) )
				{
					//printf( "adding referred shape with move (" );
					//std::cout << shape.ShapeType() << ") " << shape.HashCode( 100 ) << std::endl;
					printf( "adding referred colour: %s\n", Quantity_Color::StringName( colour.Name() ) );
					shape = XCAFDoc_ShapeTool::GetShape( ref );
					shape.Move( XCAFDoc_ShapeTool::GetLocation( label ) );
					m_shapeStack.push_back( shape );
					m_colourStack.push_back( colour );
				}
				else
				{
					printf( "what is this case????\n" );
					if ( XCAFDoc_ShapeTool::IsAssembly( label ) )
					{
						createShapeColourStacks( label );
					}
					else
					{
						Handle_XCAFDoc_ShapeTool shapeTool = XCAFDoc_DocumentTool::ShapeTool( m_doc->Main() );
						printf( "Child label information: IsShape( %d ), IsAssembly( %d ), IsFree( %d ), IsSimpleShape( %d ), IsReference( %d )\n",
							shapeTool->IsShape( label ), shapeTool->IsAssembly( label ), shapeTool->IsFree( label ), shapeTool->IsSimpleShape( label ),
							shapeTool->IsReference( label ) );
						printf( "Child referred label information: IsShape( %d ), IsAssembly( %d ), IsFree( %d ), IsSimpleShape( %d ), IsReference( %d )\n",
							shapeTool->IsShape( ref ), shapeTool->IsAssembly( ref ), shapeTool->IsFree( ref ), shapeTool->IsSimpleShape( ref ),
							shapeTool->IsReference( ref ) );
						printf( "Has children: %d\n", hasChildren( ref ) );
						if ( hasChildren( ref ) )
						{
							createShapeColourStacks( ref );
						}
						else
						{
							shape = XCAFDoc_ShapeTool::GetShape( ref );
							shape.Move( XCAFDoc_ShapeTool::GetLocation( label ) );
							m_shapeStack.push_back( shape );
							m_colourStack.push_back( colour );
						}
					}
				}
			}
		}
		else
		{
			printf( "Figure out what this case is????\n" );
		}
	}
}

void TransformOCCModel::createShapeColourStacks( const TDF_Label& label )
{
	//Handle_XCAFDoc_ShapeTool shapeTool = XCAFDoc_DocumentTool::ShapeTool( m_doc->Main() );
	//Handle_XCAFDoc_ColorTool colourTool = XCAFDoc_DocumentTool::ColorTool( m_doc->Main() );

	Quantity_Color colour;
	TopoDS_Shape S;
	TDF_Label refLabel;
	static int depth = 0;

	depth++;
	if ( hasChildren( label ) )
	{
		int count = 0;
		for ( TDF_ChildIterator it( label ); it.More(); it.Next() )
		{
			count++;
			addLabelContents( it.Value() );
		}
	}
	else
	{
		addLabelContents( label );
	}
	depth--;
}

bool TransformOCCModel::mapStacks()
{
	if ( m_shapeStack.size() == 0 )
		return false;

	if ( m_shapeStack.size() != m_colourStack.size() )
		return false;

	while ( !m_shapeStack.empty() )
	{
		m_currentColour = m_colourStack.back();
		mapShape( m_shapeStack.back() );
		m_shapeStack.pop_back();
		m_colourStack.pop_back();
	}

	return true;
}

void TransformOCCModel::printLabelInfo( const TDF_Label& label, int depth )
{
	Quantity_Color colour;
	TCollection_ExtendedString layer;
	Handle_XCAFDoc_ShapeTool shapeTool = XCAFDoc_DocumentTool::ShapeTool( m_doc->Main() );
	Handle_XCAFDoc_ColorTool colourTool = XCAFDoc_DocumentTool::ColorTool( m_doc->Main() );
	Handle_XCAFDoc_LayerTool layerTool = XCAFDoc_DocumentTool::LayerTool( m_doc->Main() );

	for ( int i = 0; i < depth; i++ )
		printf( "   " );
	printf( "Label information (level = %d) \n", depth );
	for ( int i = 0; i < depth; i++ )
		printf( "   " );
	printf( "IsShape( %d ), IsAssembly( %d ), IsFree( %d ), IsSimpleShape( %d ), IsReference( %d ) \n",
		shapeTool->IsShape( label ), shapeTool->IsAssembly( label ), shapeTool->IsFree( label ), shapeTool->IsSimpleShape( label ),
		shapeTool->IsReference( label ) );
	for ( int i = 0; i < depth; i++ )
		printf( "   " );
	printf( "IsComponent( %d ), IsCompound( %d ), IsSubShape( %d ), IsTopLevel( %d )\n",
		shapeTool->IsComponent( label ), shapeTool->IsCompound( label ), shapeTool->IsSubShape( label ),
		shapeTool->IsTopLevel( label ) );
	for ( int i = 0; i < depth; i++ )
		printf( "   " );
	//if ( colourTool->GetColor( label, colour ) )
	if ( extractColour( label, colourTool, colour ) )
	{
		std::cout << "Color( " << Quantity_Color::StringName( colour.Name() ) << " )" << std::endl;
	}
	else
	{
		std::cout << "Color( None )" << std::endl;
	}
	for ( int i = 0; i < depth; i++ )
		printf( "   " );
	if ( layerTool->GetLayer( label, layer ) )
	{
		std::cout << "Layer( " << layer << ") " << std::endl;
	}
	else
	{
		std::cout << "Layer( None )" << std::endl;
	}
}

void TransformOCCModel::traverseLabel( const TDF_Label& label )
{
	static int depth = 0;
	Handle_XCAFDoc_ShapeTool shapeTool = XCAFDoc_DocumentTool::ShapeTool( m_doc->Main() );

	depth++;
	TDF_LabelSequence subShapes;
	if ( shapeTool->GetSubShapes( label, subShapes ) )
	{
		for ( int i = 0; i < depth; i++ )
			printf( "   " );
		printf( "# sub-shapes: %d\n", subShapes.Length() );
		for ( Standard_Integer i = 1; i <= subShapes.Length(); i++ )
		{
			for ( int j = 0; j < depth; j++ )
				printf( "   " );
			printf( "sub-shape #%d\n", i );
			TDF_Label subLabel = subShapes.Value( i );
			printLabelInfo( subLabel, depth );
			traverseLabel( subLabel );
		}
	}
	if ( shapeTool->IsAssembly( label ) )
	{
		TDF_LabelSequence components;
		shapeTool->GetComponents( label, components );
		for ( int i = 0; i < depth; i++ )
			printf( "   " );
		printf( "# components: %d\n", components.Length() );
		for ( Standard_Integer i = 1; i <= components.Length(); i++ )
		{
			for ( int j = 0; j < depth; j++ )
				printf( "   " );
			printf( "component #%d\n", i );
			TDF_Label componentLabel = components.Value( i );
			printLabelInfo( componentLabel, depth );
			traverseLabel( componentLabel );
		}
	}
	if ( shapeTool->IsReference( label ) )
	{
		TDF_Label refLabel;
		shapeTool->GetReferredShape( label, refLabel );
		printLabelInfo( refLabel, depth );
		traverseLabel( refLabel );
	}
	if ( shapeTool->IsCompound( label ) )
	{
		printf( "what to do, what to do...\n" );
	}
	depth--;
}

void TransformOCCModel::basicTraversal()
{
	Handle_XCAFDoc_ShapeTool shapeTool = XCAFDoc_DocumentTool::ShapeTool( m_doc->Main() );
	TDF_LabelSequence root_labels;
	shapeTool->GetFreeShapes( root_labels );

	for ( Standard_Integer i = 1; i <= root_labels.Length(); i++ )
	{
		TDF_Label label = root_labels.Value( i );
		printLabelInfo( label, 0 );
		traverseLabel( label );
	}
}

void TransformOCCModel::recurseTopologicalShapesAndBuildGeometricShapes( TopologicalShape* shape )
{
	//std::cout << "TransformOCCModel::recurseTopologicalShape" << std::endl;
	// first clear shape map
	clearMaps();
	// then build shape map
	buildMaps( shape->shape() );
	printf( "Not working don't use error error!!! Warning Will Robinson\n" );
	// create geometric shape from topological shape map
	GeometricShape* geoShape = new GeometricShape();
	geoShape->surfaceColour( shape->surfaceColour() );
	geoShape->curveColour( shape->curveColour() );
	extractPoints( geoShape );
	extractCurvesAndSurfaces( geoShape );
	m_geometricShapes.push_back( geoShape );
	// then recurse into children
	//std::list<TopologicalShape*> children = shape->children();
	//std::cout << "child count: " << children.size() << std::endl;
	//std::list<TopologicalShape*>::iterator it = children.begin();
	//for ( ;it!=children.end();it++)
	//{
		//std::cout << "shape : " << *it << std::endl;
	//	recurseTopologicalShapesAndBuildGeometricShapes( *it );
	//}

}

void TransformOCCModel::buildGeometricShapes()
{
	//std::cout << "TransformOCCModel::buildVertexSet()" << std::endl;
	// clear maps
	clearMaps();

	// create geometric shape for every topological shape
	std::list<TopologicalShape*> children = m_rootShape.children();
	//std::cout << "root child count: " << children.size() << std::endl;
	std::list<TopologicalShape*>::iterator it = children.begin();
	for ( ;it!=children.end();it++)
		recurseTopologicalShapesAndBuildGeometricShapes( *it );

	// extract vertecis
	//nextractPoints();
}

void TransformOCCModel::labelTraversal(const TDF_Label& aLabel,
	const TopLoc_Location& aLocation, TopologicalShape* parent )
{
	//first label itself
	if ( stylePrint( aLabel, aLocation, parent ) )
	{
		//if ( parent )
		//	parent = parent->lastChild();
		//else
		//	parent = m_rootShape.lastChild();
	}
/*	Handle (TDataStd_Name) aName;
	if (aLabel.FindAttribute (TDataStd_Name::GetID(), aName))
	{
		TopoDS_Shape aShape;
		if (XCAFDoc_ShapeTool::GetShape (aLabel, aShape))
		{
			// find the position to display as middle of the bounding box
			aShape.Move (aLocation);
			Bnd_Box aBox;
			BRepBndLib::Add (aShape, aBox);
			if ( ! aBox.IsVoid() )
			{
				Standard_Real aXmin, aYmin, aZmin, aXmax, aYmax, aZmax;
				aBox.Get (aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);
				gp_Pnt aPnt (0.5 * (aXmin + aXmax), 0.5 * (aYmin + aYmax), 0.5 * (aZmin + aZmax));
				Prs3d_Text::Draw( aPrs, anAspect, aName->Get(), aPnt);
			}
		}
	}
*/

  TDF_LabelSequence seq;

	//attributes of subshapes
	if (XCAFDoc_ShapeTool::GetSubShapes (aLabel, seq))
	{
		Standard_Integer i = 1;
		for (i = 1; i <= seq.Length(); i++)
		{
			TDF_Label aL = seq.Value (i);
			labelTraversal(aL, aLocation, parent); //suppose that subshapes do not contain locations
		}
	}

	//attributes of components
	seq.Clear();
	if (XCAFDoc_ShapeTool::GetComponents (aLabel, seq))
	{
		Standard_Integer i = 1;
		for (i = 1; i <= seq.Length(); i++)
		{
			TDF_Label aL = seq.Value (i);
			labelTraversal(aL, aLocation, parent);
			//TDF_Label aRefLabel;


			// Unnecesary at least for step files this is like digging down into the part
			//attributes of referrences
			//TopLoc_Location aLoc = XCAFDoc_ShapeTool::GetLocation (aL);
			//if (XCAFDoc_ShapeTool::GetReferredShape (aL, aRefLabel))
			//{
			//	labelTraversal(aRefLabel, aLoc, parent);
			//}
			//else
			//{
			//	labelTraversal(aL, aLocation, parent );
			//}
		}
	}

}

bool TransformOCCModel::stylePrint( const TDF_Label& aLabel, const TopLoc_Location& aLocation, TopologicalShape* parent )
{
	USE_PARAMETER(aLocation);
	bool addedShape = false;
	printTDFLabel( aLabel );
	TopoDS_Shape shape;
	if ( ! XCAFDoc_ShapeTool::GetShape ( aLabel, shape ) || shape.IsNull() )
		return addedShape;

	// Catch compounds with no topological shapes
	if (shape.ShapeType() == TopAbs_COMPOUND)
	{
		TopoDS_Iterator anExplor(shape);
		if (!anExplor.More())
			return addedShape;
	}
	/*
	// find the position to display as middle of the bounding box
	shape.Move (aLocation);
	Bnd_Box aBox;
	BRepBndLib::Add (shape, aBox);
	if ( ! aBox.IsVoid() )
	{
		Standard_Real aXmin, aYmin, aZmin, aXmax, aYmax, aZmax;
		aBox.Get (aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);
		gp_Pnt aPnt (0.5 * (aXmin + aXmax), 0.5 * (aYmin + aYmax), 0.5 * (aZmin + aZmax));
		std::cout << "Location: " << aXmin << " " << aXmax << " " << aYmin << " " << aYmax << " " << aZmin << " " << aZmax << std::endl;
	}
	*/
	TCollection_AsciiString labelName;
	TDF_Tool::Entry( aLabel, labelName );

	//collect information on colored subshapes
	TopLoc_Location L;
	XCAFPrs_DataMapOfShapeStyle settings;
	XCAFPrs::CollectStyleSettings ( aLabel, L, settings );
	//std::cout << "Styles collected" << std::endl;

	// dispatch (sub)shapes by their styles
	XCAFPrs_DataMapOfStyleShape items;
	XCAFPrs_Style DefStyle;
	Quantity_Color White ( Quantity_NOC_WHITE );
	DefStyle.SetColorSurf ( White );
	DefStyle.SetColorCurv ( White );
	XCAFPrs::DispatchStyles ( shape, settings, items, DefStyle );
	//std::cout << "Dispatch done" << std::endl;

	// add subshapes to shape store (one shape per style)
	XCAFPrs_DataMapIteratorOfDataMapOfStyleShape it ( items );
	Standard_Integer i=1;
	for ( ; it.More(); it.Next() )
	{
		XCAFPrs_Style s = it.Key();
		std::cout << "Style " << i << ": [" <<
			( s.IsSetColorSurf() ? Quantity_Color::StringName ( s.GetColorSurf().Name() ) : "" ) << ", " <<
			( s.IsSetColorCurv() ? Quantity_Color::StringName ( s.GetColorCurv().Name() ) : "" ) << "]" <<
			" --> si_" << i << ( s.IsVisible() ? "" : " <invisible>" ) << std::endl;
		i++;
		// Add styled shape to store
		Cad_colour_map colourMap;
		TopologicalShape* ts = new TopologicalShape( it.Value(), colourMap );

		ts->label( labelName.ToCString() );
		if ( s.IsSetColorSurf() )
			ts->surfaceColour( s.GetColorSurf() );
		if ( s.IsSetColorCurv() )
			ts->curveColour( s.GetColorCurv() );

		addedShape = true;
		if ( parent )
		{
			printf( "Warning attempting to use dead code, don't!!!!\n" );
			//std::cout << "parent: " << parent << std::endl;
			//parent->child( ts );
		}
		else
		{
			printf( "Warning attempting to use dead code, don't!!!!\n" );
			//std::cout << "root parent: " << ts << std::endl;
			//m_rootShape.child( ts ); // root level shape
		}

	}

	return addedShape;
}

void TransformOCCModel::mapShapes( Handle_TDocStd_Document doc )
{
	clearMaps();
	if ( !m_doc.IsNull() )
		m_doc.Nullify();
	m_doc = doc;
	Handle_XCAFDoc_ShapeTool shapeTool = XCAFDoc_DocumentTool::ShapeTool( m_doc->Main() );
	Handle_XCAFDoc_ColorTool colourTool = XCAFDoc_DocumentTool::ColorTool( m_doc->Main() );
	Handle_XCAFDoc_LayerTool layerTool = XCAFDoc_DocumentTool::LayerTool( m_doc->Main() );

	Quantity_Color colour;
	TCollection_ExtendedString layer;
	TDF_LabelSequence root_labels;
	shapeTool->GetFreeShapes( root_labels );
	TopoDS_Shape S;
	TDF_Label refLabel;
	TopLoc_Location location;

	printf( "===================\n" );
	//printf( "Shape count: %d\n", root_labels.Length() );
	for ( Standard_Integer i = 1; i <= root_labels.Length(); i++ )
	{
		//plumbLabel( root_labels.Value( i ), shapeTool, colourTool );
		TDF_Label label = root_labels.Value( i );
		labelTraversal( label, location, 0 );
	//	createShapeColourStacks( label );
	//	if ( mapStacks() )
	//		printf( "Great!!\n" );
	}
	printf( "===================\n" );

	TDF_LabelSequence colourLabels;
	colourTool->GetColors( colourLabels );

	for ( int i = 1; i <= colourLabels.Length(); i++ )
	{
		if ( colourTool->GetColor( colourLabels.Value( i ), colour ) )
		{
			std::cout << Quantity_Color::StringName( colour.Name() ) << std::endl;
		}
	}

	//if ( colourLabels.Length() == 0 )
	//{
	//	std::cout << "Damn, no colours at all" << std::endl;
	//}
	//TDF_LabelSequence layerLabels;
	//layerTool->GetLayerLabels( layerLabels );
	//for ( int i = 1; i <= layerLabels.Length(); i++ )
	//{
	//	if ( layerTool->GetLayer( layerLabels.Value( i ), layer ) )
	//	{
	//		std::cout << layer << std::endl;
	//	}
	//}
	//if ( layerLabels.Length() == 0 )
	//{
	//	std::cout << "Damn, no layers at all" << std::endl;
	//}

	//printf( "???????????????????\n" );
	//basicTraversal();
	//printf( "???????????????????\n" );
}

void TransformOCCModel::mapShapes(  Handle_TopTools_HSequenceOfShape shapes  )
{
	m_doc.Nullify();
	clearMaps();
	for ( int i = 1; i <= shapes->Length(); i++ )
	{
		mapShape( shapes->Value( i ) );
	}
}

void TransformOCCModel::mapShape( TopoDS_Shape shape )
{
	bool fix_shape = false;
	if ( !shape.IsNull() )
	{
		if ( fix_shape )
		{
			ShapeFix_Shape fixer( shape );
			fixer.Perform();
			BRepBuilderAPI_Sewing sew;
			sew.Add( fixer.Shape() );
			sew.Perform();
			shape = sew.SewedShape();
		}

		buildMaps( shape );

		//healGeometry( shape, 1.e-6, true, true, false );
		extractShapeInfo();
	}
}

void TransformOCCModel::clearMaps()
{
	m_fmap.Clear();
	m_emap.Clear();
	m_vmap.Clear();
	m_shmap.Clear();
	m_somap.Clear();
	m_csmap.Clear();
	m_cmap.Clear();
}

void TransformOCCModel::buildMaps( const TopoDS_Shape& shape )
{
	TopExp::MapShapes( shape, TopAbs_VERTEX, m_vmap );
	TopExp::MapShapes( shape, TopAbs_EDGE, m_emap );
	TopExp::MapShapes( shape, TopAbs_FACE, m_fmap );
	TopExp::MapShapes( shape, TopAbs_SHELL, m_shmap );
	TopExp::MapShapes( shape, TopAbs_SOLID, m_somap );
	TopExp::MapShapes( shape, TopAbs_COMPSOLID, m_csmap );
	TopExp::MapShapes( shape, TopAbs_COMPOUND, m_cmap );
}

void TransformOCCModel::extractPoints( GeometricShape* geoShape )
{
	clock_t start, end;
	start = clock();
	int nvertices = m_vmap.Extent();
	for ( int i = 0; i < nvertices; i++ )
	{
		TopoDS_Vertex vertex = TopoDS::Vertex( m_vmap( i + 1 ) );
		gp_Pnt pnt = BRep_Tool::Pnt( vertex );
		geoShape->append( new Point( pnt ) );
	}
	end = clock();
	//printf( "Vertex (%d) extraction took %.2f seconds\n", nvertices, ( end - start ) / double( CLOCKS_PER_SEC ) );
}

void TransformOCCModel::extractCurvesAndSurfaces( GeometricShape* geoShape )
{
	clock_t start, end;
	start = clock();
	TopExp_Explorer edgeEx;
	TopTools_IndexedMapOfShape edgeMap;
	Surface* surface = 0;
	Curve* curve = 0;
	int nfaces = m_fmap.Extent();
	static int called_count = 0;
	for ( int i = 1; i <= nfaces; i++ )
	{
		TopoDS_Face face = TopoDS::Face( m_fmap( i ) );
		surface = new Surface( face );
		//surface->setColour( geoShape->surfaceColour() );
		//surface->buildOpenGLRep();
		geoShape->append( surface );
		for( edgeEx.Init(face, TopAbs_EDGE); edgeEx.More(); edgeEx.Next() )
		{
			if ( !edgeMap.Contains( edgeEx.Current() ) )
			{
				TopoDS_Edge topoDSEdge = TopoDS::Edge( edgeEx.Current() );
				edgeMap.Add( topoDSEdge );
				curve = new Curve( topoDSEdge );
				curve->setColour( geoShape->curveColour() );
				if ( !curve->is3D() )
				{
					curve->setTrimmed( face );
				}

				if ( curve->buildOpenGLRep() )
				{
					if ( curve->points() == 0 )
						printf( " wtf %d, values: %p, colours: %p, is3D(%d), is2D(%d)\n", curve->pointCount(), curve->points(), curve->colours(), curve->is3D(), curve->is2D() );
					geoShape->append( curve );
				}
				else
				{
					printf( "Curve failed to build OpenGL representation !!!!\n" );
				}
			}
		}
	}
	end = clock();
	if ( nfaces )
	{
		//printf( "Surface (%d) and curve extraction took %.2f seconds [average: %.4f]\n", nfaces, ( end - start ) / double( CLOCKS_PER_SEC ), ( end - start ) / double( CLOCKS_PER_SEC * nfaces ) );
		//printf( "curves running total: %d\n", (int)m_curves.size() );
	}
	else
	{
		printf( "No faces found!!\n" );
	}
	printf( "." );
	if ( ++called_count % 10 == 0 )
		printf( "\n" );
}

void TransformOCCModel::extractShapeInfo()
{
    // building geom vertices
    // vertices can just be considered as vertices
	clock_t start, end;
	start = clock();
	int nvertices = m_vmap.Extent();
	for ( int i = 0; i < nvertices; i++ )
	{
		TopoDS_Vertex vertex = TopoDS::Vertex( m_vmap( i + 1 ) );
		gp_Pnt pnt = BRep_Tool::Pnt( vertex );
		m_points.push_back( new Point( pnt ) );
	}
	end = clock();
	printf( "Vertex (%d) extraction took %.2f seconds\n", nvertices, ( end - start ) / double( CLOCKS_PER_SEC ) );

	Handle_XCAFDoc_ShapeTool shapeTool;
	Handle_XCAFDoc_ColorTool colourTool;
	if ( !m_doc.IsNull() )
	{
		shapeTool = XCAFDoc_DocumentTool::ShapeTool( m_doc->Main() );
		colourTool = XCAFDoc_DocumentTool::ColorTool( m_doc->Main() );
	}
	start = clock();
	TopExp_Explorer edgeEx;
	TopTools_IndexedMapOfShape edgeMap;
	Surface* surface = 0;
	Curve* curve = 0;
	Quantity_Color colour;
	int nfaces = m_fmap.Extent();
	static int curve_count = 0;
	//printf( "compound count %d\n", nc );
	for ( int i = 1; i <= nfaces; i++ )
	{
		TopoDS_Face face = TopoDS::Face( m_fmap( i ) );
		surface = new Surface( face );
		if ( !m_doc.IsNull() )
		{
				//surface->setColour( m_currentColour );
		}
		//surface->buildOpenGLRep();
		m_surfaces.push_back( surface );
		for( edgeEx.Init(face, TopAbs_EDGE); edgeEx.More(); edgeEx.Next() )
		{
			if ( !edgeMap.Contains( edgeEx.Current() ) )
			{
				TopoDS_Edge topoDSEdge = TopoDS::Edge( edgeEx.Current() );
				edgeMap.Add( topoDSEdge );
				curve_count++;
				curve = new Curve( topoDSEdge );
				if ( !m_doc.IsNull() )
				{
					curve->setColour( m_currentColour );
				}
				//curve->setColour( m_colour );
				// If not a 3D line then maybe a Line on a surface, hopefully!
				if ( !curve->is3D() )
				{
					curve->setTrimmed( face );
				}

				if ( curve->buildOpenGLRep() )
				{
					if ( curve->points() == 0 )
						printf( " wtf %d, values: %p, colours: %p, is3D(%d), is2D(%d)\n", curve->pointCount(), curve->points(), curve->colours(), curve->is3D(), curve->is2D() );
					m_curves.push_back( curve );
				}
				else
				{
					printf( "Edge failed to build OpenGL representation !!!!\n" );
				}
			}
		}
	}
	//}
	//}
	end = clock();
	if ( nfaces )
	{
		printf( "Surface (%d) and curve extraction took %.2f seconds [average: %.4f]\n", nfaces, ( end - start ) / double( CLOCKS_PER_SEC ), ( end - start ) / double( CLOCKS_PER_SEC * nfaces ) );
		printf( "curves running total: %d\n", (int)m_curves.size() );
	}
	else
	{
		printf( "No faces found!!\n" );
	}
}

void TransformOCCModel::healGeometry( TopoDS_Shape shape, double tolerance, bool fixsmalledges,
										 bool fixspotstripfaces, bool sewfaces,	bool makesolids)
{
	int nrc = 0, nrcs = 0;
	TopExp_Explorer e;
	for(e.Init(shape, TopAbs_COMPOUND); e.More(); e.Next()) nrc++;
	for(e.Init(shape, TopAbs_COMPSOLID); e.More(); e.Next()) nrcs++;

	double surfacecont = 0;

	for(int i = 1; i <= m_fmap.Extent(); i++){
		GProp_GProps system;
		BRepGProp::LinearProperties(m_fmap(i), system);
		surfacecont += system.Mass();
	}

	if(fixsmalledges){

		Handle(ShapeFix_Wire) sfw;
		Handle_ShapeBuild_ReShape rebuild = new ShapeBuild_ReShape;
		rebuild->Apply(shape);

		for(int i = 1; i <= m_fmap.Extent(); i++){
			TopExp_Explorer exp1;
			for(exp1.Init(m_fmap(i), TopAbs_WIRE); exp1.More(); exp1.Next()){
				TopoDS_Wire oldwire = TopoDS::Wire(exp1.Current());
				sfw = new ShapeFix_Wire(oldwire, TopoDS::Face(m_fmap(i)), tolerance);
				sfw->ModifyTopologyMode() = Standard_True;

				if(sfw->FixSmall(false, tolerance)){
					TopoDS_Wire newwire = sfw->Wire();
					rebuild->Replace(oldwire, newwire, Standard_False);
				}
				if((sfw->StatusSmall(ShapeExtend_FAIL1)) ||
								(sfw->StatusSmall(ShapeExtend_FAIL2)) ||
								(sfw->StatusSmall(ShapeExtend_FAIL3)))
					/** \todo write out warning message */{}
			}
		}
		shape = rebuild->Apply(shape);

		{
			Handle_ShapeBuild_ReShape rebuild = new ShapeBuild_ReShape;
			rebuild->Apply(shape);
			TopExp_Explorer exp1;
			for(exp1.Init(shape, TopAbs_EDGE); exp1.More(); exp1.Next()){
				TopoDS_Edge edge = TopoDS::Edge(exp1.Current());
				if(m_vmap.FindIndex(TopExp::FirstVertex(edge)) ==
							   m_vmap.FindIndex(TopExp::LastVertex(edge))){
					GProp_GProps system;
					BRepGProp::LinearProperties(edge, system);
					if(system.Mass() < tolerance)
					{
						rebuild->Remove(edge, false);
					}
				}
			}
			shape = rebuild->Apply(shape);
		}

		Handle(ShapeFix_Wireframe) sfwf = new ShapeFix_Wireframe;
		sfwf->SetPrecision(tolerance);
		sfwf->Load(shape);

		/** \todo replace debug messages with warnings
		if(sfwf->FixSmallEdges()){
			qDebug("- fixing wire frames");
			if(sfwf->StatusSmallEdges(ShapeExtend_OK)) qDebug("no small edges found");
			if(sfwf->StatusSmallEdges(ShapeExtend_DONE1)) qDebug("some small edges fixed");
			if(sfwf->StatusSmallEdges(ShapeExtend_FAIL1)) qDebug("failed to fix some small edges");
		}

		if(sfwf->FixWireGaps()){
			qDebug("- fixing wire gaps");
			if(sfwf->StatusWireGaps(ShapeExtend_OK)) qDebug("no gaps found");
			if(sfwf->StatusWireGaps(ShapeExtend_DONE1)) qDebug("some 2D gaps fixed");
			if(sfwf->StatusWireGaps(ShapeExtend_DONE2)) qDebug("some 3D gaps fixed");
			if(sfwf->StatusWireGaps(ShapeExtend_FAIL1)) qDebug("failed to fix some 2D gaps");
			if(sfwf->StatusWireGaps(ShapeExtend_FAIL2)) qDebug("failed to fix some 3D gaps");
		}
*/
		shape = sfwf->Shape();
	}

	if(fixspotstripfaces)
	{
		Handle(ShapeFix_FixSmallFace) sffsm = new ShapeFix_FixSmallFace;
		sffsm->Init(shape);
		sffsm->SetPrecision(tolerance);
		sffsm->Perform();

		shape = sffsm->FixShape();
	}

	if(sewfaces)
	{
		TopExp_Explorer exp0;

		BRepOffsetAPI_Sewing sewedObj(tolerance);

		for(exp0.Init(shape, TopAbs_FACE); exp0.More(); exp0.Next())
		{
			TopoDS_Face face = TopoDS::Face(exp0.Current());
			sewedObj.Add(face);
		}

		sewedObj.Perform();

		if(!sewedObj.SewedShape().IsNull())
			shape = sewedObj.SewedShape();
	}

	if(makesolids)
	{
		TopExp_Explorer exp0;

		BRepBuilderAPI_MakeSolid ms;
		int count = 0;
		for(exp0.Init(shape, TopAbs_SHELL); exp0.More(); exp0.Next())
		{
			count++;
			ms.Add(TopoDS::Shell(exp0.Current()));
		}

		if ( count )
		{
			BRepCheck_Analyzer ba(ms);
			if(ba.IsValid())
			{
				Handle(ShapeFix_Shape) sfs = new ShapeFix_Shape;
				sfs->Init(ms);
				sfs->SetPrecision(tolerance);
				sfs->SetMaxTolerance(tolerance);
				sfs->Perform();
				shape = sfs->Shape();

				for(exp0.Init(shape, TopAbs_SOLID); exp0.More(); exp0.Next())
				{
					TopoDS_Solid solid = TopoDS::Solid(exp0.Current());
					TopoDS_Solid newsolid = solid;
					BRepLib::OrientClosedSolid(newsolid);
					Handle_ShapeBuild_ReShape rebuild = new ShapeBuild_ReShape;
          // rebuild->Apply(shape);
					rebuild->Replace(solid, newsolid, Standard_False);
					TopoDS_Shape newshape = rebuild->Apply(shape, TopAbs_COMPSOLID, 1);
          // TopoDS_Shape newshape = rebuild->Apply(shape);
					shape = newshape;
				}
			}
		}
	}
}

