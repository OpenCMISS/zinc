/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "opencascadeimporter.h"

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
#include <TDataStd_TreeNode.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_Location.hxx>
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

extern "C" {
#include "api/cmiss_field_module.h"
#include "general/debug.h"
#include "general/mystring.h"
}

#include "cad/computed_field_cad_topology.h"
#include "cad/computed_field_cad_geometry.h"
#include "cad/computed_field_cad_colour.h"

#include "cad/opencascadeformatreader.h"

void performLabelAnalysis( Handle_TDocStd_Document doc, const TDF_Label& aLabel );
char *Get_default_cad_subregion_name(cmzn_region_id region);
bool modify_if_not_standard_name(char *name);

OpenCascadeImporter::OpenCascadeImporter( const char *fileName )
	: CADImporter( fileName )
{
}


OpenCascadeImporter::~OpenCascadeImporter()
{
}

bool OpenCascadeImporter::import( struct cmzn_region *region )
{
	OpenCascadeFormatReader reader;
	bool success = false;
	if ( !m_fileName.empty() )
	{
		clock_t start, end;
		start = clock();
		if ( reader.readModel( m_fileName ) )
		{
			end = clock();
			//DEBUG_PRINT( "File read took %.2f seconds\n", ( end - start ) / double( CLOCKS_PER_SEC ) );
			start = clock();
			if ( reader.hasXDEInformation() )
			{
				//success = remapDocToRegionsAndFields( reader.xDEInformation(), region );
				success = convertDocToRegionsAndFields( reader.xDEInformation(), region );
				end = clock();
				//DEBUG_PRINT( "Shape re-mapping took %.2f seconds\n", ( end - start ) / double( CLOCKS_PER_SEC ) );
			}
			else
			{
				end = clock();
				//DEBUG_PRINT( "Don't have XDE information, also no fall back plan!!!!\n" );
			}
		}
	}
	return success;
}

bool OpenCascadeImporter::labelTraversal( Handle_TDocStd_Document xdeDoc, const TDF_Label& label
	, TopLoc_Location location, cmzn_region_id parent )
{
	Handle_XCAFDoc_ShapeTool shapeTool = XCAFDoc_DocumentTool::ShapeTool( xdeDoc->Main() );
	
	int status = 0;
	char *original_name = 0;
	bool addedShape = false;

	char *name = getLabelName(label);
	append_string(&original_name, name, &status);
	bool modified_name = modify_if_not_standard_name(name);
	if (is_standard_object_name(name) == 0)
	{
		free(name);
		name = Get_default_cad_subregion_name(parent);
	}
	TDF_Label referenceLabel;
	shapeTool->GetReferredShape(label, referenceLabel);
	TDF_LabelSequence components;
	if ( shapeTool->GetComponents(label, components, (Standard_Boolean)0) )
	{
		/** If this label is an assembly */
		if (modified_name)
		{
			display_message(INFORMATION_MESSAGE, "Modified invalid region name '%s' to\n\tvalid region name '%s'\n", original_name, name);
		}
		cmzn_region_id region = cmzn_region_create_subregion(parent, name);
		if ( !region )
		{
			display_message(ERROR_MESSAGE, "OpenCascadeImporter.  "
				"Unable to create child region.  With name '%s'", name);
			free(name);

			return false;
		}

		if (components.Length() > 0)
		{
			addedShape = true;
		}
		for ( int i = 1; i <= components.Length() && addedShape; i++ )
		{
			addedShape = labelTraversal(xdeDoc, components.Value(i), location, region);
		}
		cmzn_region_destroy(&region);
	}
	else if ( !referenceLabel.IsNull() && shapeTool->GetComponents(referenceLabel, components) )
	{
		/** else if this label is a reference for an assembly */
		/** I'm expecting to have to propogate a location here */
		if (modified_name)
		{
			display_message(INFORMATION_MESSAGE, "Modified invalid region name '%s' to\n\tvalid region name '%s'\n", original_name, name);
		}
		cmzn_region_id region = cmzn_region_create_subregion(parent, name);
		if ( !region )
		{
			display_message(ERROR_MESSAGE, "OpenCascadeImporter.  "
				"Unable to create child region.  With name '%s'", name);
			free(name);

			return false;
		}

		TopLoc_Location currentLocation = shapeTool->GetLocation(label);
		location = location.Multiplied(currentLocation);

		if (components.Length() > 0)
		{
			addedShape = true;
		}
		for ( int i = 1; i <= components.Length() && addedShape; i++ )
		{
			addedShape = labelTraversal(xdeDoc, components.Value(i), location, region);
		}
		cmzn_region_destroy(&region);
	}
	else
	{
		/** else this label is just a shape or a compound shape */
		TopoDS_Shape shape;
		if ( shapeTool->GetShape(label, shape) && !shape.IsNull() )
		{
			if (shape.ShapeType() == TopAbs_COMPOUND)
			{
				performLabelAnalysis(xdeDoc, label);
				if (modified_name)
				{
					display_message(INFORMATION_MESSAGE, "Modified invalid region name '%s' to\n\tvalid region name '%s'\n", original_name, name);
				}
				cmzn_region_id region = cmzn_region_create_subregion(parent, name);
				if ( !region )
				{
					display_message(ERROR_MESSAGE, "OpenCascadeImporter.  "
						"Unable to create child region '%s'.", name);
					free(name);

					return false;
				}
				addedShape = addCompoundShapeToRegion(xdeDoc, shape, location, region);
				cmzn_region_destroy(&region);
			}
			else
			{
				addedShape = addLabelsShapeToRegion( xdeDoc, label, location, parent );
			}
		}
	}
	if (original_name)
		DEALLOCATE(original_name);
	free(name);

	return addedShape;
}

void performLabelAnalysis( Handle_TDocStd_Document xdeDoc, const TDF_Label& aLabel )
{
	Handle_XCAFDoc_ShapeTool shapeTool = XCAFDoc_DocumentTool::ShapeTool( xdeDoc->Main() );
	Handle_XCAFDoc_ColorTool colorTool = XCAFDoc_DocumentTool::ColorTool( xdeDoc->Main() );
	TCollection_AsciiString entry;
	TDF_Tool::Entry( aLabel, entry );
	std::cout << entry.ToCString() << " ";
	if ( shapeTool->IsAssembly(aLabel) )
	{
		std::cout << "- Assembly ";
	}
	if ( shapeTool->IsComponent(aLabel) )
	{
		std::cout << "- Component ";
	}
	if ( shapeTool->IsCompound(aLabel) )
	{
		std::cout << "- Compound ";
	}
	if ( shapeTool->IsFree(aLabel) )
	{
		std::cout << "- Free ";
	}
	if ( shapeTool->IsReference(aLabel) )
	{
		std::cout << "- Reference ";
	}
	if ( shapeTool->IsShape(aLabel) )
	{
		std::cout << "- Shape ";
	}
	if ( shapeTool->IsSimpleShape(aLabel) )
	{
		std::cout << "- SimpleShape ";
	}
	if ( shapeTool->IsSubShape(aLabel) )
	{
		std::cout << "- SubShape ";
	}
	if ( shapeTool->IsTopLevel(aLabel) )
	{
		std::cout << "- TopLevel ";
	}
	TDF_LabelSequence labels;
	if ( shapeTool->GetUsers(aLabel, labels) )
	{
		std::cout << "- Users ";
	}
	TopLoc_Location location = shapeTool->GetLocation(aLabel);
	if ( !location.IsIdentity() )
	{
		std::cout << "- Location ";
	}
	Quantity_Color color;
	if ( colorTool->GetColor(aLabel, color) )
	{
		std::cout << "- color ";
	}
	if ( aLabel.HasChild() )
	{
		std::cout << "- Parent ";
	}
	if ( aLabel.HasAttribute() )
	{
		
		std::cout << "- Attribute " << aLabel.NbAttributes() << " ";
		Handle_TDataStd_Name name = 0;
		if ( aLabel.FindAttribute(TDataStd_Name::GetID(), name) )
		{
			std::cout << " Found " << aLabel.Depth() << " " << name->Get();
		}
	}
	std::cout << std::endl;
}

cmzn_cad_colour::cmzn_cad_colour_type examineShapeForColor( Handle_TDocStd_Document xdeDoc, const TopoDS_Shape& shape, TopAbs_ShapeEnum /*type*/ )
{
	XCAFDoc_ColorType colorTypes[3] = {XCAFDoc_ColorGen, XCAFDoc_ColorSurf, XCAFDoc_ColorCurv};
	Handle_XCAFDoc_ColorTool colorTool = XCAFDoc_DocumentTool::ColorTool( xdeDoc->Main() );
	Quantity_Color aColor;
	for ( int j = 0; j < 3; j++ )
	{
		if ( colorTool->IsSet(shape, colorTypes[j]) )
		{
			colorTool->GetColor(shape, colorTypes[j], aColor);
			//std::cout << "Yes we have a colour " << j << " " << type << " and it is ";
			//std::cout << "[ " << aColor._CSFDB_GetQuantity_ColorMyRed() << ", ";
			//std::cout << aColor._CSFDB_GetQuantity_ColorMyGreen() << ", ";
			//std::cout << aColor._CSFDB_GetQuantity_ColorMyBlue() << " ]" << std::endl;
			return (cmzn_cad_colour::cmzn_cad_colour_type)j;
		}
	}

	return cmzn_cad_colour::CMZN_CAD_COLOUR_NOT_DEFINED;
}

cmzn_cad_colour::cmzn_cad_colour_type examineShapeForColor( Handle_TDocStd_Document xdeDoc, const TopoDS_Shape& shape, Quantity_Color &aColor )
{
	XCAFDoc_ColorType colorTypes[3] = {XCAFDoc_ColorGen, XCAFDoc_ColorSurf, XCAFDoc_ColorCurv};
	Handle_XCAFDoc_ColorTool colorTool = XCAFDoc_DocumentTool::ColorTool( xdeDoc->Main() );
	for ( int j = 0; j < 3; j++ )
	{
		if ( colorTool->IsSet(shape, colorTypes[j]) )
		{
			colorTool->GetColor(shape, colorTypes[j], aColor);
			return (cmzn_cad_colour::cmzn_cad_colour_type)j;
		}
	}

	return cmzn_cad_colour::CMZN_CAD_COLOUR_NOT_DEFINED;
}

void updateColourMapFromShape(Handle_TDocStd_Document xdeDoc, Cad_colour_map& colourMap, const TopoDS_Shape& shape)
{
	Quantity_Color color = Quantity_NOC_WHITE;
	cmzn_cad_colour::cmzn_cad_colour_type colour_type = cmzn_cad_colour::CMZN_CAD_COLOUR_NOT_DEFINED;
	{
		TopExp_Explorer Ex1, Ex2, Ex3, Ex4, Ex5, Ex6;
		int solid_index = 0;
		for (Ex1.Init(shape,TopAbs_SOLID); Ex1.More(); Ex1.Next()) 
		{
			examineShapeForColor(xdeDoc, Ex1.Current(), TopAbs_SOLID);
			colour_type = examineShapeForColor(xdeDoc, Ex1.Current(), color);
			if (colour_type != cmzn_cad_colour::CMZN_CAD_COLOUR_NOT_DEFINED)
			{
				Cad_topology_primitive_identifier cad_id;
				cmzn_cad_colour cad_colour(colour_type, color);
				colourMap.insert(std::pair<Cad_topology_primitive_identifier,cmzn_cad_colour>(cad_id, cad_colour));
			}
			solid_index++;
			for (Ex2.Init(Ex1.Current(),TopAbs_SHELL); Ex2.More(); Ex2.Next()) 
			{
				examineShapeForColor(xdeDoc, Ex2.Current(), TopAbs_SHELL);
				int face_index = 0;
				for (Ex3.Init(Ex2.Current(),TopAbs_FACE); Ex3.More(); Ex3.Next()) 
				{
					//DEBUG_PRINT("Checking face %d, on surface %d\n", face_index, solid_index);
					examineShapeForColor(xdeDoc, Ex3.Current(), TopAbs_FACE);
					colour_type = examineShapeForColor(xdeDoc, Ex3.Current(), color);
					if (colour_type != cmzn_cad_colour::CMZN_CAD_COLOUR_NOT_DEFINED)
					{
						Cad_topology_primitive_identifier cad_id(face_index);
						cmzn_cad_colour cad_colour(colour_type, color);
						colourMap.insert(std::pair<Cad_topology_primitive_identifier,cmzn_cad_colour>(cad_id, cad_colour));
					}
					face_index++;
					for (Ex4.Init(Ex3.Current(),TopAbs_WIRE); Ex4.More(); Ex4.Next()) 
					{
						examineShapeForColor(xdeDoc, Ex4.Current(), TopAbs_WIRE);
						int edge_index = 0;
						for (Ex5.Init(Ex4.Current(),TopAbs_EDGE); Ex5.More(); Ex5.Next()) 
						{
							examineShapeForColor(xdeDoc, Ex5.Current(), TopAbs_EDGE);
							colour_type = examineShapeForColor(xdeDoc, Ex4.Current(), color);
							if (colour_type != cmzn_cad_colour::CMZN_CAD_COLOUR_NOT_DEFINED)
							{
								Cad_topology_primitive_identifier cad_id(face_index, edge_index);
								cmzn_cad_colour cad_colour(colour_type, color);
								colourMap.insert(std::pair<Cad_topology_primitive_identifier,cmzn_cad_colour>(cad_id, cad_colour));
							}
							edge_index++;
							for (Ex6.Init(Ex5.Current(),TopAbs_VERTEX); Ex6.More(); Ex6.Next()) 
							{
								examineShapeForColor(xdeDoc, Ex6.Current(), TopAbs_VERTEX);
							}
						}
					}
				}
			}
		}
	}
}

Cad_colour_map createColourMap( Handle_TDocStd_Document xdeDoc, const TDF_Label& label )
{
	Handle_XCAFDoc_ShapeTool shapeTool = XCAFDoc_DocumentTool::ShapeTool( xdeDoc->Main() );

	Cad_colour_map colourMap;
	// First look at shape itself
	TopoDS_Shape shape = shapeTool->GetShape(label);
	updateColourMapFromShape(xdeDoc, colourMap, shape);

	// Second look at referred shape
	TDF_Label referenceLabel;
	shapeTool->GetReferredShape(label, referenceLabel);
	if ( !referenceLabel.IsNull() )
	{
		TopoDS_Shape referredShape = shapeTool->GetShape(referenceLabel);
		updateColourMapFromShape(xdeDoc, colourMap, referredShape);
	}
	
	//DEBUG_PRINT("colour map size %d\n", colourMap.size());

	return colourMap;
}

char *Next_cad_field_name_in_series(cmzn_field_module_id field_module, const char *name)
{
	cmzn_field_id existing_field;
	int field_number = 1;
	char name_buffer[255];
	int n = sprintf(name_buffer, "%s_%d", name, field_number);
	while (n > 0 && n < 250 && (NULL != (existing_field = cmzn_field_module_find_field_by_name(field_module, name_buffer))))
	{
		field_number++;
		cmzn_field_destroy(&existing_field);
		n = sprintf(name_buffer, "%s_%d", name, field_number);
	}

	if (n > 0 && n < 250 && (existing_field == NULL))
	{
		char *next_name = duplicate_string(name_buffer);
		return next_name;
	}

	return NULL;
}

int Set_default_cad_field_name(cmzn_field_id field, cmzn_field_module_id field_module)
{
	int return_code = 0;
	cmzn_field_id existing_field;
	int field_number = 1;
	char name_buffer[255];
	int n = sprintf(name_buffer, "cad_field_%d", field_number);
	while (n > 0 && n < 250 && (NULL != (existing_field = cmzn_field_module_find_field_by_name(field_module, name_buffer))))
	{
		field_number++;
		cmzn_field_destroy(&existing_field);
		n = sprintf(name_buffer, "cad_field_%d", field_number);
	}

	if (n > 0 && n < 250 && (existing_field == NULL))
	{
		cmzn_field_set_name(field, name_buffer);
		return_code = 1;
	}

	return return_code;
}

char *Get_default_cad_subregion_name(cmzn_region_id region)
{
	cmzn_region_id existing_region;
	int region_number = 1;
	char name_buffer[255];
	int n = sprintf(name_buffer, "cad_region_%d", region_number);
	while(n > 0 && n < 250 && (NULL != (existing_region = cmzn_region_find_child_by_name(region, name_buffer))))
	{
		region_number++;
		cmzn_region_destroy(&existing_region);
		n = sprintf(name_buffer, "cad_region_%d", region_number);
	}

	if (n > 0 && n < 250 && (existing_region == NULL))
	{
		char *name = (char *)malloc(sizeof(char) * (n+1));
		name = strncpy(name_buffer, name_buffer, n+1);
		name[n] = '\0';
		return name;
	}

	return NULL;
}

/**
 * test if the string is empty.  This extends zero length
 * strings to include those that are entirely made up of spaces
 *
 * @param name the string to test
 * @return true if the string has zero length or zero 
 * non-whitespace characters, false otherwise
 */
bool is_empty_string(char *name)
{
	bool empty = true;
	int length = strlen(name);
	for (int i = 0; i < length; i++)
	{
		if (!isspace(name[i]))
		{
			return false;
		}
	}

	return empty;
}

bool modify_if_not_standard_name(char *name)
{	
	bool modified = false;

	if (is_standard_object_name(name) == 0 && !is_empty_string(name))
	{
		// Try to modify this name so that it conforms
		modified = true;
		if (!isalnum(name[0]))
		{
			name[0] = 'm';
		}
		int length = strlen(name);
		for (int i = 1; i < length; i++)
		{
			if ((name[i] != '_') && (name[i] != '.') && (name[i] != ':') && (!isalnum(name[i])))
			{
				name[i] = '_';
			}
		}
	}

	return modified;
}

char *OpenCascadeImporter::getLabelName(const TDF_Label& label)
{
	char *string_name = 0;
	Handle_TDataStd_Name name = 0;
	if ( label.FindAttribute(TDataStd_Name::GetID(), name) )
	{
		string_name = (char *) malloc( ( name->Get().LengthOfCString() + 1 ) * sizeof( char ) );
		name->Get().ToUTF8CString( string_name );
	}

	return string_name;
}

bool OpenCascadeImporter::addCompoundShapeToRegion(Handle_TDocStd_Document xdeDoc, const TopoDS_Shape& shape, TopLoc_Location location, cmzn_region_id parent)
{
	bool addedShape = false;
	TopoDS_Iterator anExplorer(shape);
	static int depth = 0;
	int count = 0;
	for (int i = 0; i < depth; i++)
	{
		printf("-");
	}
	while(anExplorer.More())
	{
		printf("%d (%d) ", count++, anExplorer.Value().ShapeType());
		TopoDS_Shape subShape = anExplorer.Value();
		if (subShape.ShapeType() == TopAbs_COMPOUND)
		{
			depth++;
			char *name = Get_default_cad_subregion_name(parent);
			cmzn_region_id region = cmzn_region_create_subregion(parent, name);
			if ( !region )
			{
				display_message(ERROR_MESSAGE, "OpenCascadeImporter.  "
					"Unable to create child region '%s'.", name);
				free(name);
				return false;
			}
			free(name);
			addedShape = addCompoundShapeToRegion(xdeDoc, subShape, location, region);
			cmzn_region_destroy(&region);
		}
		else
		{
			// Can I add this shape to the region???
			printf("add \n\t");
			Cad_colour_map colour_map;
			const char name[] = "";
			addedShape = addShapeToRegion(subShape, parent, name, colour_map);
		}
		anExplorer.Next();
	}
	if (depth == 1)
		printf("\n");
	depth--;

	return addedShape;
}

bool OpenCascadeImporter::addShapeToRegion(const TopoDS_Shape& shape, cmzn_region_id region, const char *name, const Cad_colour_map& colourMap)
{
	bool addedShape = false;

	TopologicalShape* ts = new TopologicalShape( shape, colourMap );

	// Create field and add to current region
	char *top_name = 0, *geo_name = 0, *col_name = 0;
	char *next_name = 0, *temp_name = 0;
	int status = 0;
	cmzn_field_module_id field_module = cmzn_region_get_field_module(region);
	cmzn_field_id cad_topology_field = cmzn_field_module_create_cad_topology( field_module, ts );
	//if (name && (strlen(name) > 0) )
	append_string(&temp_name, name, &status);
	if (modify_if_not_standard_name(temp_name))
	{
		display_message(INFORMATION_MESSAGE, "Modifying invalid field name '%s' to\n\tvalid field name '%s'\n", name, temp_name);
		next_name = Next_cad_field_name_in_series(field_module, temp_name);
	}
	else if (is_standard_object_name(name))
	{
		next_name = Next_cad_field_name_in_series(field_module, name);
	}
	else
	{	
		const char default_cad_field_name[] = "cad_field";
		next_name = Next_cad_field_name_in_series(field_module, default_cad_field_name);
	}
	DEALLOCATE(temp_name);
	append_string(&top_name, next_name, &status);
	//append_string(&top_name, "_top", &status);
	cmzn_field_set_name(cad_topology_field, top_name);

	cmzn_field_set_managed(cad_topology_field, 1);
	cmzn_field_cad_topology_id cad_topology = cmzn_field_cast_cad_topology( cad_topology_field );

	// Create shape gs
	GeometricShape* gs = new GeometricShape();
	ts->tessellate(gs);
	cmzn_field_cad_topology_set_geometric_shape(cad_topology, gs);
	// sketchy:
	cmzn_field_destroy((cmzn_field_id *)&cad_topology);

	append_string(&geo_name, next_name, &status);
	append_string(&geo_name, "_geo", &status);
	append_string(&col_name, next_name, &status);
	append_string(&col_name, "_col", &status);

	cmzn_field_id cad_geometry_field = Computed_field_module_create_cad_geometry(field_module, cad_topology_field);
	cmzn_field_set_name(cad_geometry_field, geo_name);
	cmzn_field_set_managed(cad_geometry_field, 1);

	cmzn_field_id cad_colour_field = Computed_field_module_create_cad_colour(field_module, cad_topology_field);
	cmzn_field_set_name(cad_colour_field, col_name);
	cmzn_field_set_managed(cad_colour_field, 1);

	if ( cad_topology_field && cad_geometry_field && cad_colour_field )
	{
		addedShape = true;
	}
	cmzn_field_destroy(&cad_colour_field);
	cmzn_field_destroy(&cad_geometry_field);
	cmzn_field_destroy(&cad_topology_field);

	DEALLOCATE(next_name);
	DEALLOCATE(top_name);
	DEALLOCATE(geo_name);
	DEALLOCATE(col_name);

	cmzn_field_module_destroy(&field_module);

	return addedShape;
}

bool OpenCascadeImporter::addLabelsShapeToRegion( Handle_TDocStd_Document xdeDoc, const TDF_Label& label, TopLoc_Location location, cmzn_region_id parent )
{
	bool addedShape = false;
	Handle_XCAFDoc_ShapeTool shapeTool = XCAFDoc_DocumentTool::ShapeTool( xdeDoc->Main() );
	TopoDS_Shape shape;
	if ( !shapeTool->GetShape(label, shape) || shape.IsNull() )
		return addedShape;

	// Catch compounds with no topological shapes
	if (shape.ShapeType() == TopAbs_COMPOUND)
	{
		display_message(INFORMATION_MESSAGE, "addLabelsShapeToRegion() test for COMPOUND shape, shouldn't get here because I am already looking for it earlier???\n");
		performLabelAnalysis(xdeDoc, label);
		TDF_Label referenceLabel;
		shapeTool->GetReferredShape(label, referenceLabel);
		performLabelAnalysis(xdeDoc, referenceLabel);
		TopoDS_Iterator anExplorer(shape);
		if (!anExplorer.More())
		{
			display_message(INFORMATION_MESSAGE, "and it contains no shapes\n");
			return addedShape;
		}
	}

	char *string_name = 0;
	if (shapeTool->IsReference(label))
	{
		//DEBUG_PRINT("Getting a name from a referred label.\n");
		TDF_Label referenceLabel;
		shapeTool->GetReferredShape(label, referenceLabel);
		string_name = getLabelName(referenceLabel);
	}
	else
	{
		string_name = getLabelName(label);
	}

	Cad_colour_map colourMap = createColourMap(xdeDoc, label);
	shape.Move(location);
	
	addedShape = addShapeToRegion(shape, parent, string_name, colourMap);

	if (string_name)
		free(string_name);

	return addedShape;
}

int iterateOverLabel( Handle_TDocStd_Document xdeDoc, const TDF_Label& label, int depth )
{
	depth++;

	Handle_XCAFDoc_ShapeTool shapeTool = XCAFDoc_DocumentTool::ShapeTool( xdeDoc->Main() );
	TDF_LabelSequence labels;
	performLabelAnalysis(xdeDoc, label);
	if ( shapeTool->IsAssembly(label) )
	{
		TDF_LabelSequence components;
		//Standard_Boolean isAssembly = shapeTool->GetComponents(label, components, (Standard_Boolean)0);

		for ( int i = 1; i <= components.Length(); i++ )
		{
			if ( depth < 10 )
				iterateOverLabel(xdeDoc, components.Value(i), depth);
		}
	}
	//else if ( shapeTool->IsReference(label) )
	//{
	//	TDF_Label referenceLabel;
	//	shapeTool->GetReferredShape(label, referenceLabel);
	//	if ( depth < 10 )
	//		iterateOverLabel(xdeDoc, referenceLabel, depth);
	//}
	//else if ( shapeTool->GetSubShapes(label, labels) )
	//{
	//	std::cout << "Sub-shapes" << std::endl;
	//	for ( int i = 1; i <= labels.Length(); i++ )
	//	{
	//		if ( depth < 10 )
	//			iterateOverLabel(xdeDoc, labels.Value(i), depth);
	//	}
	//}

	{
		//DEBUG_PRINT("Color hunt ");
		performLabelAnalysis(xdeDoc, label);
		TopExp_Explorer Ex1, Ex2, Ex3, Ex4, Ex5, Ex6;
		TopoDS_Shape shape = shapeTool->GetShape(label);
		for (Ex1.Init(shape,TopAbs_SOLID); Ex1.More(); Ex1.Next()) 
		{
			examineShapeForColor(xdeDoc, Ex1.Current(), TopAbs_SOLID);
			if ( shapeTool->IsReference(label) )
			{
				TDF_Label referenceLabel;
				shapeTool->GetReferredShape(label, referenceLabel);
				TopoDS_Shape referenceShape = shapeTool->GetShape(referenceLabel);
				examineShapeForColor(xdeDoc, referenceShape, TopAbs_SOLID);
			}
			for (Ex2.Init(Ex1.Current(),TopAbs_SHELL); Ex2.More(); Ex2.Next()) 
			{
				examineShapeForColor(xdeDoc, Ex2.Current(), TopAbs_SHELL);
				for (Ex3.Init(Ex2.Current(),TopAbs_FACE); Ex3.More(); Ex3.Next()) 
				{
					examineShapeForColor(xdeDoc, Ex3.Current(), TopAbs_FACE);
					for (Ex4.Init(Ex3.Current(),TopAbs_WIRE); Ex4.More(); Ex4.Next()) 
					{
						examineShapeForColor(xdeDoc, Ex4.Current(), TopAbs_WIRE);
						for (Ex5.Init(Ex4.Current(),TopAbs_EDGE); Ex5.More(); Ex5.Next()) 
						{
							examineShapeForColor(xdeDoc, Ex5.Current(), TopAbs_EDGE);
							for (Ex6.Init(Ex5.Current(),TopAbs_VERTEX); Ex6.More(); Ex6.Next()) 
							{
								examineShapeForColor(xdeDoc, Ex6.Current(), TopAbs_VERTEX);
							}
						}
					}
				}
			}
		}
	}

	return depth;
}

bool OpenCascadeImporter::convertDocToRegionsAndFields( Handle_TDocStd_Document xdeDoc, cmzn_region_id region )
{
	bool addedShape = false;
	if ( !xdeDoc )
		return addedShape;

	Handle_XCAFDoc_ShapeTool shapeTool = XCAFDoc_DocumentTool::ShapeTool( xdeDoc->Main() );
	Handle_XCAFDoc_ColorTool colorTool = XCAFDoc_DocumentTool::ColorTool( xdeDoc->Main() );

	TopLoc_Location location;
	TDF_LabelSequence freeShapes;
	shapeTool->GetFreeShapes( freeShapes );

#if !defined (OPTIMISED)
	Quantity_Color aColor;
	TDF_LabelSequence colLabels;
	colorTool->GetColors(colLabels);
	//DEBUG_PRINT( "Colours in xdeDoc\n");
	for ( int i = 1; i <= colLabels.Length(); i++)
	{
		const TDF_Label& aLabel = colLabels.Value(i);
		if ( colorTool->GetColor(aLabel, aColor) )
		{
			TCollection_AsciiString entry;
			TDF_Tool::Entry( aLabel, entry );
			//Standard_CString cString = entry.ToCString();
			//DEBUG_PRINT("%d. %5.3f %5.3f %5.3f (%s)\n",i , aColor._CSFDB_GetQuantity_ColorMyRed(),
			//	aColor._CSFDB_GetQuantity_ColorMyGreen(), aColor._CSFDB_GetQuantity_ColorMyBlue(), cString);
			//std::cout << "(" << cString << ")" << std::endl;
		}
	}
	//DEBUG_PRINT("\n\n");
#endif /* !defined (OPTIMISED) */
	cmzn_region_begin_change(region);
	for ( Standard_Integer i = 1; i <= freeShapes.Length(); i++ )
	{
		TDF_Label label = freeShapes.Value( i );

#if !defined (OPTIMISED)
		TCollection_AsciiString entry;
		TDF_Tool::Entry( label, entry );
		//DEBUG_PRINT( "root region '%s'\n", entry.ToCString() );
#endif /* !defined (OPTIMISED) */
		if ( label.IsNull() )
			continue;

		addedShape = labelTraversal( xdeDoc, label, location, region );
	}
	cmzn_region_end_change(region);

	return addedShape;
}

int cmzn_region_import_cad_file(struct cmzn_region *region, const char *file_name)
{
	int return_code = 0;

	if (region && file_name)
	{
		OpenCascadeImporter occImporter( file_name );
		if ( occImporter.import( region ) )
		{
			return_code = 1;
		}
		else
		{
			display_message( ERROR_MESSAGE, "Failed to load file '%s' into region '%s'", file_name, cmzn_region_get_name(region) );
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_region_import_cad_file.  Invalid argument(s)\n");
	}

	return return_code;
}

