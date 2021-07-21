/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "opencascadeformatreader.h"

#include <TopoDS_Shape.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <IGESControl_Reader.hxx>
#include <STEPControl_Reader.hxx>
#include <FSD_File.hxx>
//#include <ShapeSchema.hxx>
#include <Storage_Root.hxx>
#include <Storage_HSeqOfRoot.hxx>
//#include <PTopoDS_HShape.hxx>
#include <PTColStd_PersistentTransientMap.hxx>
//#include <MgtBRep.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <IGESCAFControl_Reader.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TDocStd_Document.hxx>

extern "C"
{
#include "general/debug.h"
}

OpenCascadeFormatReader::OpenCascadeFormatReader()
	: m_sequenceOfShapes()
	, m_doc()
{
}


OpenCascadeFormatReader::~OpenCascadeFormatReader()
{
		m_sequenceOfShapes.Nullify();
		m_doc.Nullify();
}

OpenCascadeFormatReader::FileFormat OpenCascadeFormatReader::determineFileFormat( const std::string& fileName )
{
	FileFormat format = FormatUnknown;
	if ( fileName.rfind( ".brep" ) != std::string::npos ||
		fileName.rfind( ".rle" ) != std::string::npos )
	{
		format = FormatBREP;
	}
	else if ( fileName.rfind( ".step" ) != std::string::npos ||
		fileName.rfind( ".STEP" ) != std::string::npos ||
		fileName.rfind( ".stp" ) != std::string::npos ||
		fileName.rfind( ".STP" ) != std::string::npos )
	{
		format = FormatSTEP;
	}
	else if ( fileName.rfind( ".iges" ) != std::string::npos ||
		fileName.rfind( ".IGES" ) != std::string::npos ||
		fileName.rfind( ".igs" ) != std::string::npos ||
		fileName.rfind( ".IGS" ) != std::string::npos )
	{
		format = FormatIGES;
	}
	else if ( fileName.rfind( ".vrml" ) != std::string::npos )
	{
		format = FormatVRML;
	}
	else if ( fileName.rfind( ".stl" ) != std::string::npos )
	{
		format = FormatSTL;
	}
	else if ( fileName.rfind( ".dxf" ) != std::string::npos )
	{
		format = FormatDXF;
	}
	else if ( fileName.rfind( ".csfdb" ) != std::string::npos )
	{
		format = FormatCSFDB;
	}

	return format;
}

bool OpenCascadeFormatReader::readModel( const std::string& fileName )
{
	FileFormat format = determineFileFormat( fileName );
	bool result = false;
	try {
		switch ( format )
		{
		case FormatBREP:
			result = importBREP( fileName );
			break;
		case FormatIGES:
			result = importIGES( fileName );
			break;
		case FormatSTEP:
			result = importSTEP( fileName );
			break;
		case FormatCSFDB:
		case FormatVRML:
		case FormatSTL:
		case FormatDXF:
			/** @todo Implement more import file formats */
			printf( "This file format is not yet implemented\n" );
			break;
		case FormatUnknown:
			printf( "The format of file '%s' is unknown\n", fileName.c_str() );
			m_sequenceOfShapes.Nullify();
			break;
		default:
			printf( "Failed to determine format of file '%s'\n", fileName.c_str() );
			m_sequenceOfShapes.Nullify();
			// To Do - Error message here?
			break;
		}
	} catch ( Standard_Failure ) {
		printf( "Import failed\n" );
		m_sequenceOfShapes.Nullify();
	}
    
	return result;
}

bool OpenCascadeFormatReader::importBREP( const std::string& file )
{
	TopoDS_Shape aShape;
	BRep_Builder aBuilder;
	
	m_sequenceOfShapes = new TopTools_HSequenceOfShape();

	Standard_Boolean result = BRepTools::Read(  aShape, file.c_str(), aBuilder );
	if ( result )
	{
		m_sequenceOfShapes->Append( aShape );
		return true;
	}

	return false;
}

bool OpenCascadeFormatReader::importIGES( const std::string& file )
{
	//IGESControl_Reader reader;
#if defined (_DEBUG)
	std::cout << "importIGES( " << file << " )" << std::endl;
#endif /* defined (_DEBUG) */
	m_sequenceOfShapes.Nullify();
	m_doc.Nullify();
	//m_doc = new TDocStd_Document( "MDTV-XCAF" );
	XCAFApp_Application::GetApplication()->NewDocument("MDTV-XCAF", m_doc);

#if defined (_DEBUG)
	if (XCAFDoc_DocumentTool::IsXCAFDocument(m_doc))
	{
		std::cout << "Yep She's structured for use with XDE" << std::endl;
	}
	else
	{
		std::cout << "No way She's not structured for use with XDE" << std::endl;
	}
#endif /* defined (_DEBUG) */
	IGESCAFControl_Reader reader;

	// Should already be set but hey.
	reader.SetColorMode( Standard_True );
	reader.SetNameMode( Standard_True );
	reader.SetLayerMode( Standard_True );

	const char *c_string = file.c_str();
	int status = reader.ReadFile( c_string );

	if ( status == IFSelect_RetDone )
	{
		if ( !reader.Transfer( m_doc ) )
		{
			// If transfer to xcaf document fails then grab the shapes and save
			// them as a sequence of shapes, all higher level information is lost
			// at this point
			m_doc.Nullify();
			m_sequenceOfShapes = new TopTools_HSequenceOfShape();
			std::cout << "doc transfer failure" << std::endl;
			TopoDS_Shape aShape = reader.OneShape();
			m_sequenceOfShapes->Append( aShape );
		}
	}
	else
	{
		m_sequenceOfShapes.Nullify();
	}

	return ( status == IFSelect_RetDone );
}

bool OpenCascadeFormatReader::importSTEP( const std::string& file )
{
	m_sequenceOfShapes.Nullify();
	m_doc.Nullify();

	//m_doc = new TDocStd_Document( "MDTV-XCAF" );
	XCAFApp_Application::GetApplication()->NewDocument("MDTV-XCAF", m_doc);

	if (XCAFDoc_DocumentTool::IsXCAFDocument(m_doc))
	{
		//DEBUG_PRINT("Yep She's structured for use with XDE\n");
	}
	else
	{
		//DEBUG_PRINT("No way She's not structured for use with XDE\n");
	}
	STEPCAFControl_Reader reader;

	// Should already be set but hey.
	reader.SetColorMode( Standard_True );
	reader.SetNameMode( Standard_True );
	reader.SetLayerMode( Standard_True );
	IFSelect_ReturnStatus status = reader.ReadFile( file.c_str() );
	if ( status == IFSelect_RetDone )
	{
		if ( !reader.Transfer( m_doc ) )
		{
			// If the transfer to the xcaf document fails then just grab the shapes
			// and save them as a sequence of shapes, all higher level information is 
			// lost at this point
			m_doc.Nullify();
			m_sequenceOfShapes = new TopTools_HSequenceOfShape();
			Standard_Integer count = reader.Reader().TransferRoots();
			// Standard_Integer count = reader.ChangeReader().TransferRoots(); // OCE 6.5.0
			if (count)
			{
				TopoDS_Shape shape = reader.Reader().OneShape();
				m_sequenceOfShapes->Append( shape );
			}
			/*
			bool failsonly = false;
			aReader.Reader().PrintCheckLoad( failsonly, IFSelect_ItemsByEntity );
		
			int nbr = aReader.Reader().NbRootsForTransfer();
			aReader.Reader().PrintCheckTransfer( failsonly, IFSelect_ItemsByEntity );
			for ( Standard_Integer n = 1; n <= nbr; n++ )
			{
				bool ok = aReader.Reader().TransferRoot( n );
				int nbs = aReader.Reader().NbShapes();
				if ( ok && nbs > 0 )
				{
					for ( int i = 1; i <= nbs; i++ )
					{
						TopoDS_Shape shape = reader.Reader().Shape( i );
						m_sequenceOfShapes->Append( shape );
					}
				}
			}
			*/
		}
	}
	return ( status == IFSelect_RetDone );
}

