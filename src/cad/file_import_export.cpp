/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "fileimportexport.h"

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
#include <IGESCAFControl_Reader.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TDocStd_Document.hxx>

FileImportExport::FileImportExport()
	: m_sequenceOfShapes()
{
}


FileImportExport::~FileImportExport()
{
}

bool FileImportExport::readModel( const std::string& fileName, FileFormat format )
{
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
				default:
					printf( "Failed to determine format of file '%s'\n", fileName.c_str() );
					m_sequenceOfShapes.Nullify();
					// To Do - Error message here?
					break;
				}
		} catch ( Standard_Failure ) {
				m_sequenceOfShapes.Nullify();
		}
		
		return result;
}

bool FileImportExport::importBREP( const std::string& file )
{
	TopoDS_Shape aShape;
	BRep_Builder aBuilder;
	m_sequenceOfShapes.Nullify();
	m_sequenceOfShapes = new TopTools_HSequenceOfShape();

	Standard_Boolean result = BRepTools::Read( aShape, file.c_str(), aBuilder );
	if ( result )
	{
		m_sequenceOfShapes->Append( aShape );
	}

	return (result == (Standard_Boolean)1);
}

bool FileImportExport::importIGES( const std::string& file )
{
	//IGESControl_Reader reader;
	std::cout << "importIGES( " << file << " )" << std::endl;
	m_sequenceOfShapes.Nullify();
	m_doc.Nullify();
	m_doc = new TDocStd_Document( "MDTV-XCAF" );

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
		//m_doc.Nullify();
		//reader.TransferRoots();
	}
	else
	{
		m_sequenceOfShapes.Nullify();
	}

	return ( status == IFSelect_RetDone );
}

bool FileImportExport::importSTEP( const std::string& file )
{
	m_sequenceOfShapes.Nullify();
	m_doc.Nullify();

	m_doc = new TDocStd_Document( "MDTV-XCAF" );

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
			printf( "Transferred %d roots\n", count );
			printf( "Number of roots for transfer %d\n", reader.Reader().NbRootsForTransfer() );
			printf( "Number of shapes %d\n", reader.Reader().NbShapes() );
			TopoDS_Shape shape = reader.Reader().OneShape();
			m_sequenceOfShapes->Append( shape );
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

