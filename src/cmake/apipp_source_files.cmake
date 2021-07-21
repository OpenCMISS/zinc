
# Defines APIPP_SRCS, APIPP_TYPES_HDRS

# OpenCMISS-Zinc Library
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

SET( APIPP_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/context.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/differentialoperator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/element.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/elementbasis.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/elementfieldtemplate.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/elementtemplate.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldcache.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/field.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldmodule.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldalias.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldarithmeticoperators.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldassignment.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldcomposite.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldconditional.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldconstant.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldcoordinatetransformation.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldderivatives.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldfibres.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldfiniteelement.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldgroup.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldimage.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldimageprocessing.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldlogicaloperators.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldmatrixoperators.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldmeshoperators.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldnodesetoperators.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldparameters.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldsceneviewerprojection.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldsmoothing.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldsubobjectgroup.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldtime.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldtrigonometry.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/fieldvectoroperators.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/glyph.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/graphics.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/font.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/light.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/logger.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/material.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/mesh.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/node.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/nodeset.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/nodetemplate.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/optimisation.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/region.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/result.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/scene.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/scenefilter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/scenepicker.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/sceneviewer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/sceneviewerinput.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/shader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/selection.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/spectrum.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/status.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/stream.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/streamimage.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/streamregion.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/streamscene.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/tessellation.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/timenotifier.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/timekeeper.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/timesequence.hpp
    )

SET( APIPP_TYPES_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/api/opencmiss/zinc/types/scenecoordinatesystem.hpp
    )

