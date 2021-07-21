/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef FILEIOMANAGER_H
#define FILEIOMANAGER_H

#include <string>

#include <Handle_TopTools_HSequenceOfShape.hxx>
#include <Handle_TDocStd_Document.hxx>
#include <TDF_Label.hxx>
/**
	@author user <hsorby@eggzachary>
*/
class FileImportExport
{
public:
    enum FileFormat{ FormatUnknown, FormatBREP, FormatIGES, FormatSTEP, FormatCSFDB, FormatVRML, FormatSTL, FormatDXF };
    
    FileImportExport();
    ~FileImportExport();
    
    bool readModel( const std::string& fileName, FileFormat format );
		
		bool hasSequenceOfShape() const { return ( !m_sequenceOfShapes.IsNull() ); }
		bool hasXDEInformation() const { return ( !m_doc.IsNull() ); }

		Handle_TopTools_HSequenceOfShape sequenceOfShapes() const { return m_sequenceOfShapes; }
		Handle_TDocStd_Document xDEInformation() const { return m_doc; }

private:
    bool importBREP( const std::string& file );
    bool importIGES( const std::string& file );
    bool importSTEP( const std::string& file );

private:
	Handle_TopTools_HSequenceOfShape m_sequenceOfShapes;
	Handle_TDocStd_Document m_doc;
};

#endif
