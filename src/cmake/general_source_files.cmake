
# Defines GENERAL_SRCS

# OpenCMISS-Zinc Library
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

SET( GENERAL_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/general/callback.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/child_process.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/compare.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/debug.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/error_handler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/geometry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/image_utilities.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/indexed_multi_range.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/integration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/io_stream.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/machine.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/matrix_vector.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/message.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/message_log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/multi_range.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/myio.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/mystring.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/octree.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/statistics.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/time.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/value.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stream/stream_private.cpp )
SET( GENERAL_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/general/block_array.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/callback.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/callback_class.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/callback_private.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/change_log.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/change_log_private.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/child_process.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/cmiss_set.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/compare.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/debug.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/enumerator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/enumerator_conversion.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/enumerator_private.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/enumerator_private.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/error_handler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/geometry.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/image_utilities.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/indexed_list_private.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/indexed_list_stl_private.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/indexed_multi_range.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/integration.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/io_stream.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/list.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/list_object_with_list_member_private.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/list_private.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/machine.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/manager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/manager_private.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/math.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/matrix_vector.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/message.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/message_log.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/multi_range.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/myio.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/mystring.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/object.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/octree.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/random.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/refcounted.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/refhandle.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/general/simple_list.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/statistics.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/time.h
  ${CMAKE_CURRENT_SOURCE_DIR}/general/value.h
  ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/json.h
  ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/json-forwards.h
  ${CMAKE_CURRENT_SOURCE_DIR}/stream/stream_private.hpp )
IF( NOT HAVE_VFSCANF )
	SET( GENERAL_SRCS ${GENERAL_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/general/alt_vfscanf.c )
	SET( GENERAL_HDRS ${GENERAL_HDRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/general/alt_vfscanf.h )
ENDIF( NOT HAVE_VFSCANF )
IF( NOT HAVE_HEAPSORT )
	SET( GENERAL_SRCS ${GENERAL_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/general/heapsort.cpp )
	SET( GENERAL_HDRS ${GENERAL_HDRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/general/heapsort.h )
ENDIF( NOT HAVE_HEAPSORT )
IF( ${GRAPHICS_API} MATCHES OPENGL_GRAPHICS )
  SET( GENERAL_SRCS ${GENERAL_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/general/photogrammetry.cpp )
  SET( GENERAL_HDRS ${GENERAL_HDRS} ${CMAKE_CURRENT_SOURCE_DIR}/general/photogrammetry.h )
ENDIF( ${GRAPHICS_API} MATCHES OPENGL_GRAPHICS )

