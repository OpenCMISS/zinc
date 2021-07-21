/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef OPENCASCADEIMPORTER_H
#define OPENCASCADEIMPORTER_H

#include <TopoDS_Shape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <Handle_TopTools_HSequenceOfShape.hxx>
#include <Handle_TDocStd_Document.hxx>
#include <Handle_XCAFDoc_ShapeTool.hxx>
#include <Handle_XCAFDoc_ColorTool.hxx>
#include <Quantity_Color.hxx>
#include <TDF_Label.hxx>

#include <vector>

#include "cad/entity.h"
#include "cad/point.h"
#include "cad/curve.h"
#include "cad/surface.h"
#include "cad/geometricshape.h"
#include "cad/topologicalshape.h"
#include "cad/topologicalshaperoot.h"
#include "cad/cadimporter.h"

/**
 * A class to facilitate importing data from files via OpenCASCADE into a region.
 */
class OpenCascadeImporter : public CADImporter
{
public:
	/**
	 * Create an OpenCASCADE importer, optionally given a file name.
	 *
	 * @param fileName a file name to import data from, optionally set here.
	 */
	OpenCascadeImporter( const char *fileName = "" );
	~OpenCascadeImporter();

	/**
	 * Import the file passed to the constructor or set with fileName(const char *name)
	 * into the given region.
	 *
	 * @param the region to import the data from the file into
	 * @returns true if successful, false otherwise
	 * @see #fileName(const char *name)
	 */
	bool import( cmzn_region_id region );

private:
	/**
	 * Convert an OpenCASCADE Extended Data Exchange (XDE) document into regions and fields.
	 * An XDE document is internally managed as a DOM document, there exists a limited API to
	 * query, analyse and extract information contained in the document.  New version using
	 * shape colour search.
	 *
	 * @param xdeDoc a handle to a valid XDE document
	 * @param region the region to create cad defined regions and fields into
	 * @returns true if successful, false otherwise
	 */
	bool convertDocToRegionsAndFields(Handle_TDocStd_Document xdeDoc, cmzn_region_id region);

	/**
	 * Add a labels shape as cad topology computed field to the given region.  Here we also add the geometric representation
	 * of the shape attached to the given label, for displaying, and also we create a colour representation 
	 * for the shape (if available) so that the shape can be visualised as expected.
	 *
	 * @param xdeDoc a handle to a valid XDE document
	 * @param label the current document label
	 * @param location the current location
	 * @param parent the region to create cad defined regions and fields into
	 * @returns true if successful, false otherwise
	 */
	bool addLabelsShapeToRegion(Handle_TDocStd_Document xdeDoc, const TDF_Label& label, TopLoc_Location location , cmzn_region_id parent);

	/**
	 * Add a compound shape as a cad topology computed field to the given region.
	 */
	bool addCompoundShapeToRegion(Handle_TDocStd_Document xdeDoc, const TopoDS_Shape& shape, TopLoc_Location location, cmzn_region_id parent);

	/**
	 * Add a shape as a cad topology computed field to the given region.
	 */
	bool addShapeToRegion(const TopoDS_Shape& shape, cmzn_region_id parent, const char *name, const Cad_colour_map& colourMap);
	
	/**
	 * Traverse the labels in an XDE document.  This is necessary when dealing with assemblies, an assembly doesn't contain any shape 
	 * information it is only a container for other assemblies or shapes.
	 * @param xdeDoc a handle to a valid XDE document
	 *
	 * @param label the current document label
	 * @param location the current location
	 * @param parent the region to create cad defined regions and fields into
	 * @returns true if successful, false otherwise
	 */
	bool labelTraversal(Handle_TDocStd_Document xdeDoc, const TDF_Label& label, TopLoc_Location location, cmzn_region_id parent);

	/**
	 * Get the label name if it exists, this may return an empty string ""
	 *
	 * @param label the label to get name attribute from
	 * @returns a char * that points to the name that has been allocated, must be freed
	 */
	char *getLabelName(const TDF_Label& label);
};

#endif
