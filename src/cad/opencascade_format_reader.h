/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef OPENCASCADEFORMATREADER_H
#define OPENCASCADEFORMATREADER_H

#include <string>

#include <Handle_TopTools_HSequenceOfShape.hxx>
#include <Handle_TDocStd_Document.hxx>
#include <TDF_Label.hxx>
/**
	@author user <hsorby@eggzachary>
	@ingroup CAD
	@brief Reads different formats of CAD files using OpenCascade
*/
class OpenCascadeFormatReader
{
public:
	enum FileFormat{ FormatUnknown, FormatBREP, FormatIGES, FormatSTEP, FormatCSFDB, FormatVRML, FormatSTL, FormatDXF };
	
	OpenCascadeFormatReader();
	~OpenCascadeFormatReader();
	
	bool readModel( const std::string& fileName );
	
	bool hasSequenceOfShape() const { return ( !m_sequenceOfShapes.IsNull() ); }
	bool hasXDEInformation() const { return ( !m_doc.IsNull() ); }

	Handle_TopTools_HSequenceOfShape sequenceOfShapes() const { return m_sequenceOfShapes; }
	Handle_TDocStd_Document xDEInformation() const { return m_doc; }

private:
	FileFormat determineFileFormat( const std::string& fileName );
	bool importBREP( const std::string& file );
	bool importIGES( const std::string& file );
	bool importSTEP( const std::string& file );

private:
	Handle_TopTools_HSequenceOfShape m_sequenceOfShapes;
	Handle_TDocStd_Document m_doc;
};

#endif
