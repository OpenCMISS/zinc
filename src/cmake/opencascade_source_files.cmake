
# Defines OPENCASCADE_SRCS

# OpenCMISS-Zinc Library
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

IF( USE_OPENCASCADE )
	SET( OPENCASCADE_SRCS 
    #${CMAKE_CURRENT_SOURCE_DIR}/cad/occpartfactory.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/cad/graphicimporter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/cad_element.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/cad_tool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/entity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/element_identifier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/point.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/curve.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/surface.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/cad/transformoccmodel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/geometricshape.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/topologicalshape.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/cad/topologicalshaperoot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/cadimporter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/opencascadeimporter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/opencascadeformatreader.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/cad/fileimportexport.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/computed_field_cad_topology.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/computed_field_cad_geometry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/computed_field_cad_colour.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/field_location.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/cad_geometry_to_graphics_object.cpp )
	
	SET( OPENCASCADE_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/cad_element.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/cad_geometry_to_graphics_object.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/cad_tool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/curve.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/point.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/cadimporter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/entity.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/element_identifier.h
    #${CMAKE_CURRENT_SOURCE_DIR}/cad/occpartfactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/surface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/field_location.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/topologicalshape.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/computed_field_cad_colour.h
    #${CMAKE_CURRENT_SOURCE_DIR}/cad/fileimportexport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/opencascadeformatreader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/topologicalshaperoot.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/computed_field_cad_geometry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/transformoccmodel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/computed_field_cad_topology.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/geometricshape.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/opencascadeimporter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cad/computed_field_cad_topology_private.h )
    #${CMAKE_CURRENT_SOURCE_DIR}/cad/graphicimporter.h )

ENDIF( USE_OPENCASCADE )

