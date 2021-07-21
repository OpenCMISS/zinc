
# OpenCMISS-Zinc Library
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Defines COMFILE_SRCS, ELEMENT_SRCS, EMOTER_SRCS, FINITE_ELEMENT_CORE_SRCS, FINITE_ELEMENT_GRAPHICS_SRCS,
# FINITE_ELEMENT_SRCS (definition includes the previous two), INTERACTION_SRCS, IO_DEVICES_SRCS, LICENSE_HDRS, NODE_SRCS,
# REGION_SRCS, SELECTION_SRCS, THREE_D_DRAWING_SRCS, TIME_SRCS

SET( DATASTORE_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/datastore/labels.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/datastore/labelschangelog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/datastore/labelsgroup.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/datastore/map.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/datastore/mapindexing.cpp )
SET( DATASTORE_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/datastore/labels.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/datastore/labelschangelog.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/datastore/labelsgroup.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/datastore/map.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/datastore/maparray.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/datastore/mapindexing.hpp )

SET( ELEMENT_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/element/element_operations.cpp )
SET( ELEMENT_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/element/element_operations.h )

SET( EMOTER_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/emoter/em_cmgui.cpp )
SET( EMOTER_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/emoter/em_cmgui.h )

SET( INTERACTION_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/interaction/interaction_graphics.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/interaction/interaction_volume.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/interaction/interactive_event.cpp )
SET( INTERACTION_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/interaction/interaction_graphics.h
  ${CMAKE_CURRENT_SOURCE_DIR}/interaction/interaction_volume.h
  ${CMAKE_CURRENT_SOURCE_DIR}/interaction/interactive_event.h )

SET( FIELD_IO_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/field_io/fieldml_common.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/field_io/read_fieldml.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/field_io/write_fieldml.cpp )
SET( FIELD_IO_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/field_io/fieldml_common.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/field_io/read_fieldml.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/field_io/write_fieldml.hpp )

SET( IMAGE_IO_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/image_io/analyze.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/image_io/analyze_object_map.cpp )
SET( IMAGE_IO_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/image_io/analyze.h
  ${CMAKE_CURRENT_SOURCE_DIR}/image_io/analyze_header.h
  ${CMAKE_CURRENT_SOURCE_DIR}/image_io/analyze_object_map.hpp )

SET( LICENSE_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/license.h )

SET( MESH_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/mesh/cmiss_element_private.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mesh/cmiss_node_private.cpp )
SET( MESH_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/mesh/cmiss_node_private.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mesh/cmiss_element_private.hpp )

SET( NODE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/node/node_operations.cpp )
SET( NODE_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/node/node_operations.h )

SET( REGION_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/region/cmiss_region.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stream/region_stream.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/region/cmiss_region_write_info.cpp )
SET( REGION_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/region/cmiss_region.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stream/region_stream.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/region/cmiss_region_write_info.h )

SET( SELECTION_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/selection/element_point_ranges_selection.cpp )
SET( SELECTION_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/selection/element_point_ranges_selection.h )

SET( TIME_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/timekeeper_json_io.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/time/time.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/time/time_keeper.cpp )
SET( TIME_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/description_io/timekeeper_json_io.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/time/time.h
  ${CMAKE_CURRENT_SOURCE_DIR}/time/time_keeper.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/time/time_private.h )

SET( THREE_D_DRAWING_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/three_d_drawing/graphics_buffer.cpp )
SET( THREE_D_DRAWING_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/three_d_drawing/graphics_buffer.h )
IF( ${GRAPHICS_API} MATCHES OPENGL_GRAPHICS )
  #SET( THREE_D_DRAWING_SRCS ${THREE_D_DRAWING_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/general/photogrammetry.cpp )
  SET( THREE_D_DRAWING_HDRS ${THREE_D_DRAWING_HDRS} ${CMAKE_CURRENT_SOURCE_DIR}/three_d_drawing/abstract_graphics_buffer.h )
ENDIF( ${GRAPHICS_API} MATCHES OPENGL_GRAPHICS )
