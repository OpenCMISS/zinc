/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "graphicimporter.h"

#include <string>
#include <time.h>

#include "occpartfactory.h"
#include "transformoccmodel.h"
#include "fileimportexport.h"

extern "C" {
#include "user_interface/message.h"
}

GraphicImporter::GraphicImporter( const char *fileName, const char *shapeName )
  : m_transformer( new TransformOCCModel )
  , m_fileName( std::string( fileName ) )
  , m_shapeName( std::string( shapeName ) )
{
}


GraphicImporter::~GraphicImporter()
{
	if ( m_transformer )
		delete m_transformer;
}

const char *GraphicImporter::name()
{
  if ( m_fileName.empty() )
    return m_shapeName.c_str();

  return m_fileName.c_str();
}

bool GraphicImporter::import()
{
  bool success = false;
  if ( m_fileName.empty() )
  {
    TopoDS_Shape shape;
    if ( m_shapeName.compare( "cube" ) == 0 )
    {
      shape = OCCPartFactory::makeCube( 10, 10, 10 );
    }
		if ( m_shapeName.compare( "cylinder" ) == 0 )
		{
			shape = OCCPartFactory::makeCylinder( 5, 10 );
		}
		if ( m_shapeName.compare( "bottle" ) == 0 )
		{
			try {
				shape = OCCPartFactory::makeBottle( 50, 70, 30 );
			} catch ( Standard_ConstructionError ) {
				shape.Nullify();
				display_message( WARNING_MESSAGE, "Houston, we have a problem with the bottle" );
			}
		}
    m_transformer->mapShape( shape );
    if ( !shape.IsNull() )
      success = true;
  }
	else
	{
		FileImportExport::FileFormat format = FileImportExport::FormatUnknown;
		FileImportExport reader;
		if ( m_fileName.rfind( ".brep" ) != std::string::npos ||
			m_fileName.rfind( ".rle" ) != std::string::npos )
		{
			format = FileImportExport::FormatBREP;
		}
		else if ( m_fileName.rfind( ".step" ) != std::string::npos ||
			m_fileName.rfind( ".stp" ) != std::string::npos )
		{
			format = FileImportExport::FormatSTEP;
		}
		else if ( m_fileName.rfind( ".iges" ) != std::string::npos ||
			m_fileName.rfind( ".igs" ) != std::string::npos )
		{
			format = FileImportExport::FormatIGES;
		}

		clock_t start, end;
		start = clock();
		Handle_TopTools_HSequenceOfShape shapes;
		if ( reader.readModel( m_fileName, format ) )
		{
			end = clock();
			printf( "File read took %.2f seconds\n", ( end - start ) / double( CLOCKS_PER_SEC ) );
			start = clock();
			if ( reader.hasXDEInformation() )
				m_transformer->mapShapes( reader.xDEInformation() );
			else
				m_transformer->mapShapes( reader.sequenceOfShapes() );
			end = clock();
			printf( "Shape mapping took %.2f seconds\n", ( end - start ) / double( CLOCKS_PER_SEC ) );
			success = true;
		}
	}
  return success;
}

void GraphicImporter::buildGeometricShapes()
{
	m_transformer->buildGeometricShapes();
}

const std::vector<GeometricShape*>& GraphicImporter::geometricShapes() const
{
	return m_transformer->geometricShapes();
}

int GraphicImporter::pointCount() const
{
  return m_transformer->pointCount();
}

const Point* GraphicImporter::point( int index ) const
{
  return m_transformer->point( index );
}

int GraphicImporter::curveCount() const
{
	return m_transformer->curveCount();
}

const Curve* GraphicImporter::curve( int index ) const
{
	return m_transformer->curve( index );
}

int GraphicImporter::surfaceCount() const
{
	return m_transformer->surfaceCount();
}

const Surface* GraphicImporter::surface( int index ) const
{
	return m_transformer->surface( index );
}

