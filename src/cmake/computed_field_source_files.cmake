
# Defines COMPUTED_FIELD_CORE_SRCS, COMPUTED_FIELD_GRAPHICS_SRCS, COMPUTED_FIELD_SRCS (contains previous two)

# OpenCMISS-Zinc Library
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

SET( COMPUTED_FIELD_CORE_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_arithmetic_operators.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_composite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_conditional.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_coordinate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_fibres.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_format_output.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_function.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_group.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_logical_operators.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_matrix_operators.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_mesh_operators.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_nodeset_operators.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_scene_viewer_projection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_subobject_group.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_string_constant.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_trigonometry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_vector_operators.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_wrappers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/differential_operator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/field_cache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/field_derivative.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/field_module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/fieldassignmentprivate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/fieldparametersprivate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/fieldsmoothingprivate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_find_xi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_finite_element.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_set.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/field_location.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/field_json_io.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/fieldmodule_json_io.cpp )
SET( COMPUTED_FIELD_CORE_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_arithmetic_operators.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_composite.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_conditional.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_coordinate.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_format_output.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_function.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_group.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_group_base.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_logical_operators.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_matrix_operators.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_mesh_operators.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_nodeset_operators.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_scene_viewer_projection.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_subobject_group.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_string_constant.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_trigonometry.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_vector_operators.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_wrappers.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/differential_operator.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/field_cache.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/field_derivative.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/field_module.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/fieldassignmentprivate.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/fieldparametersprivate.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/fieldsmoothingprivate.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_find_xi.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_finite_element.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_set.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/field_location.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/field_json_io.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/fieldmodule_json_io.hpp )

SET( COMPUTED_FIELD_GRAPHICS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_find_xi_graphics.cpp )
SET( COMPUTED_FIELD_GRAPHICS_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_find_xi_graphics.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_find_xi_private.hpp )

SET( COMPUTED_FIELD_SRCS
	${COMPUTED_FIELD_CORE_SRCS}
	${COMPUTED_FIELD_GRAPHICS_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/minimise/minimise.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/minimise/cmiss_optimisation_private.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/minimise/optimisation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_alias.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_compose.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_deformation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_image.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_integration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_lookup.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_time.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_update.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_value_index_ranges.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stream/field_image_stream.cpp )

SET( COMPUTED_FIELD_HDRS
	${COMPUTED_FIELD_CORE_HDRS}
	${COMPUTED_FIELD_GRAPHICS_HDRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/minimise/minimise.h
  ${CMAKE_CURRENT_SOURCE_DIR}/minimise/cmiss_optimisation_private.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/minimise/optimisation.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_alias.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_compose.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_deformation.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_external.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_image.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_image_processing.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_integration.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_lookup.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_private.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_time.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_update.h
  ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_value_index_ranges.h
  ${CMAKE_CURRENT_SOURCE_DIR}/stream/field_image_stream.hpp )

IF( ZINC_USE_ITK )
  SET( COMPUTED_FIELD_SRCS ${COMPUTED_FIELD_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_derivatives.cpp )
  SET( COMPUTED_FIELD_HDRS ${COMPUTED_FIELD_HDRS} ${CMAKE_CURRENT_SOURCE_DIR}/computed_field/computed_field_derivatives.h )
ENDIF( ZINC_USE_ITK )


