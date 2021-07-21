/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CADIMPORTER_H
#define CADIMPORTER_H
/**
 * @defgroup CAD This module is used to incorporate CAD abilities into CMGUI
 */
#include <string>

extern "C" {
#include "region/cmiss_region.h"
}

/**
	@author user <hsorby@eggzachary>
	@ingroup CAD
	@brief Class to import CAD data via OpenCascade.
*/
class CADImporter
{
public:
	/**
	 * Constructor takes a file name for ease of use
	 * @paramin file name, defaults to empty string
	 */
  CADImporter( const char *fileName = "" );
  virtual ~CADImporter();

	/**
	 * Get the current file name
	 * @return file name
	 */
  const char *fileName();

	/**
	 * Set the filename to import CAD data from
	 * @paramin file name
	 */
	void fileName( const char* name );

	/**
	 * Import CAD shapes
	 * Uses the tried and tested method of suffix testing to determine file format
	 * @return true if at least one shape was imported, false otherwise
	 */
  virtual bool import( struct cmzn_region *region ) = 0;
  
protected:
  std::string m_fileName;
};

#endif

