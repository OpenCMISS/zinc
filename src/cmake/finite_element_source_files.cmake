
# Defines FINITE_ELEMENT_CORE_SRCS, FINITE_ELEMENT_GRAPHICS_SRCS,
# FINITE_ELEMENT_ADDITIONAL_SRCS, FINITE_ELEMENT_SRCS
# (group definition which includes the previous three)

# OpenCMISS-Zinc Library
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

SET( FINITE_ELEMENT_CORE_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/element_field_template.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/export_finite_element.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_basis.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_discretization.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_field.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_field_evaluation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_field_parameters.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_mesh.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_nodeset.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_region.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_shape.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_time.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_value_storage.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/import_finite_element.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/node_field_template.cpp )
SET( FINITE_ELEMENT_CORE_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/element_field_template.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/export_finite_element.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_basis.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_constants.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_discretization.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_field.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_field_evaluation.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_field_parameters.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_field_private.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_mesh.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_nodeset.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_private.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_region.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_region_private.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_shape.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_time.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_value_storage.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/import_finite_element.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/node_field_template.hpp )

SET( FINITE_ELEMENT_GRAPHICS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_to_graphics_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_to_iso_lines.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_to_iso_surfaces.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_to_streamlines.cpp )
SET( FINITE_ELEMENT_GRAPHICS_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_to_graphics_object.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_to_iso_lines.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_to_iso_surfaces.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_to_streamlines.h )

SET( FINITE_ELEMENT_ADDITIONAL_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/export_cm_files.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_adjacent_elements.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_conversion.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_to_iges.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/snake.cpp )
SET( FINITE_ELEMENT_ADDITIONAL_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/export_cm_files.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_adjacent_elements.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_conversion.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/finite_element_to_iges.h
  ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/snake.h )

IF( ZINC_USE_NETGEN )
	SET( FINITE_ELEMENT_CORE_SRCS
		${FINITE_ELEMENT_CORE_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/generate_mesh_netgen.cpp )
	SET( FINITE_ELEMENT_CORE_HDRS
		${FINITE_ELEMENT_CORE_HDRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/finite_element/generate_mesh_netgen.h )
ENDIF( ZINC_USE_NETGEN )

SET( FINITE_ELEMENT_SRCS
	${FINITE_ELEMENT_CORE_SRCS}
	${FINITE_ELEMENT_GRAPHICS_SRCS}
	${FINITE_ELEMENT_ADDITIONAL_SRCS} )
SET(  FINITE_ELEMENT_HDRS
	${FINITE_ELEMENT_CORE_HDRS}
	${FINITE_ELEMENT_GRAPHICS_HDRS}
	${FINITE_ELEMENT_ADDITIONAL_HDRS} )

