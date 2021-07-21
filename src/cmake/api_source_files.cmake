
# Defines API_HDRS

# OpenCMISS-Zinc Library
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

SET( API_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/context.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/core.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/differentialoperator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/element.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/elementbasis.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/elementfieldtemplate.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/elementtemplate.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/field.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldalias.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldarithmeticoperators.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldassignment.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldcache.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldcomposite.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldconditional.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldconstant.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldcoordinatetransformation.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldderivatives.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldfibres.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldfiniteelement.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldgroup.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldimage.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldimageprocessing.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldlogicaloperators.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldmatrixoperators.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldmeshoperators.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldmodule.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldnodesetoperators.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldparameters.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldsceneviewerprojection.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldsmoothing.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldsubobjectgroup.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldtime.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldtrigonometry.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldvectoroperators.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/glyph.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/graphics.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/font.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/light.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/logger.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/material.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/mesh.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/node.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/nodeset.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/nodetemplate.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/optimisation.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/region.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/result.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/scene.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/scenefilter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/scenepicker.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/selection.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/shader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/spectrum.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/status.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/stream.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/streamimage.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/streamregion.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/streamscene.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/tessellation.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/timenotifier.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/timekeeper.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/timesequence.h)

SET( API_TYPES_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/cinline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/contextid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/differentialoperatorid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/elementid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/elementbasisid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/elementfieldtemplateid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/elementtemplateid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldaliasid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldcacheid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldcompositeid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldconstantid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldfiniteelementid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldgroupid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldassignmentid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldimageid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldimageprocessingid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldmatrixoperatorsid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldmeshoperatorsid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldmoduleid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldnodesetoperatorsid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldparametersid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldsmoothingid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldsubobjectgroupid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fontid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/glyphid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/graphicsid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/lightid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/loggerid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/materialid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/meshid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/nodeid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/nodesetid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/nodetemplateid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/optimisationid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/regionid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/scenecoordinatesystem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/scenefilterid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/sceneid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/scenepickerid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/selectionid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/shaderid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/spectrumid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/streamid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/tessellationid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/timekeeperid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/timenotifierid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/timesequenceid.h )

IF( USE_OPENCASCADE )
	SET( API_HDRS ${API_HDRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldcad.h)
	SET( API_TYPES_HDRS ${API_TYPES_HDRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/fieldcadid.h )
ENDIF( USE_OPENCASCADE )

IF( ${GRAPHICS_API} MATCHES OPENGL_GRAPHICS )
	SET( API_HDRS ${API_HDRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/sceneviewer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/sceneviewerinput.h)
	SET( API_TYPES_HDRS ${API_TYPES_HDRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/sceneviewerid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/sceneviewerinputid.h )
ENDIF( ${GRAPHICS_API} MATCHES OPENGL_GRAPHICS )

