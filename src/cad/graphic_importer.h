/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef GRAPHICIMPORTER_H
#define GRAPHICIMPORTER_H

#include <string>
#include <vector>

class TransformOCCModel;
class GeometricShape;
class Point;
class Curve;
class Surface;
/**
	@author user <hsorby@eggzachary>
*/
class GraphicImporter
{
public:
  GraphicImporter( const char *fileName, const char *shapeName );
  ~GraphicImporter();

  const char *name();
  bool import();
  
	void buildGeometricShapes();
	const std::vector<GeometricShape*>& geometricShapes() const;

  int pointCount() const;
  const Point* point( int index ) const;

	int curveCount() const;
	const Curve* curve( int index ) const;

	int surfaceCount() const;
	const Surface* surface( int index ) const;

private:
  TransformOCCModel *m_transformer;
  std::string m_fileName;
  std::string m_shapeName;
};

#endif

