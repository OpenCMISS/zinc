/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "cad/cadimporter.h"

extern "C" {
#include "user_interface/message.h"
}

CADImporter::CADImporter( const char *fileName )
  : m_fileName( std::string( fileName ) )
{
}

CADImporter::~CADImporter()
{
}

const char *CADImporter::fileName()
{
  return m_fileName.c_str();
}

void CADImporter::fileName( const char *name )
{
	m_fileName = std::string( name );
}

