# OpenCMISS-Zinc Library
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Set the minimum version of cmake required to 2.8.1
# The FATAL_ERROR is needed if using cmake < 2.4

IF( MSVC )
    SET( ZINC_C_INLINE __inline )
ELSE()
    SET( ZINC_C_INLINE "static inline" )
ENDIF()

set(FILES_FOR_CLEANING)

set( ZINC_CONFIGURE_HDR ${CMAKE_CURRENT_BINARY_DIR}/api/opencmiss/zinc/zincconfigure.h )
set( ZINC_SHARED_OBJECT_HDR ${CMAKE_CURRENT_BINARY_DIR}/api/opencmiss/zinc/zincsharedobject.h )
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/configure/zincconfigure.h.cmake
    ${ZINC_CONFIGURE_HDR} )
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/configure/zincsharedobject.h.cmake
    ${ZINC_SHARED_OBJECT_HDR} )
set( ZINC_CONFIGURE_HDRS ${ZINC_CONFIGURE_HDR} ${ZINC_SHARED_OBJECT_HDR} )
list(APPEND FILES_FOR_CLEANING ${ZINC_CONFIGURE_HDR} ${ZINC_SHARED_OBJECT_HDR} ${CMAKE_CURRENT_BINARY_DIR}/source)

# Load in source files, each source file list may contain several
# CMake variables see comments for a list of _SRCS included from
# a particular file (similarly that file defines a _HDRS variable).
# Defines API_HDRS, API_TYPES_HDRS
include(cmake/api_source_files.cmake )

# Defines APIPP_HDRS, APIPP_TYPES_HDRS
include(cmake/apipp_source_files.cmake )

# Defines COMMAND_SRCS, CONTEXT_SRCS
include(cmake/command_source_files.cmake )

# Defines COMPUTED_FIELD_CORE_SRCS, COMPUTED_FIELD_GRAPHICS_SRCS,
# COMPUTED_FIELD_SRCS (contains previous two)
include(cmake/computed_field_source_files.cmake )

# Defines FINITE_ELEMENT_CORE_SRCS, FINITE_ELEMENT_GRAPHICS_SRCS,
# FINITE_ELEMENT_ADDITIONAL_SRCS, FINITE_ELEMENT_SRCS
# (group definition which includes the previous three)
include(cmake/finite_element_source_files.cmake )

# Defines GENERAL_SRCS
include(cmake/general_source_files.cmake )

# Defines GRAPHICS_SRCS
include(cmake/graphics_source_files.cmake )

# Defines IMAGE_PROCESSING_SRCS
include(cmake/image_processing_source_files.cmake )

# Defines COMFILE_SRCS, ELEMENT_SRCS, EMOTER_SRCS
# FIELD_IO_SRCS, INTERACTION_SRCS, IO_DEVICES_SRCS, MESH_SRCS, NODE_SRCS,
# REGION_SRCS, SELECTION_SRCS, THREE_D_DRAWING_SRCS, TIME_SRCS
include(cmake/misc_source_files.cmake )

# Defines OPENCASCADE_SRCS
#include(cmake/opencascade_source_files.cmake )

# Defines ZINC_LIB_SRCS, ZINC_LIB_HDRS
include(cmake/zinc_lib_source_files.cmake)

# Generate the version.cpp / version.h files
set(ZINC_VERSION_STAGING_SRC ${CMAKE_CURRENT_BINARY_DIR}/version.staged.cpp)
set(ZINC_VERSION_SRC ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)
if (OPENCMISS_HAVE_MULTICONFIG_ENV)
    set(CONFIGURE_BUILD_TYPE $<CONFIG>)
else ()
    set(CONFIGURE_BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif ()
configure_file(${PROJECT_SOURCE_DIR}/cmake/configure_version_cpp_file.in.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/configure_version_cpp_file.cmake @ONLY)
add_custom_command(OUTPUT ${ZINC_VERSION_STAGING_SRC}
    COMMAND "${CMAKE_COMMAND}" -DACTIVE_BUILD_TYPE=${CONFIGURE_BUILD_TYPE} -P ${CMAKE_CURRENT_BINARY_DIR}/configure_version_cpp_file.cmake
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${ZINC_VERSION_STAGING_SRC} ${ZINC_VERSION_SRC}
    COMMAND "${CMAKE_COMMAND}" -E remove ${ZINC_VERSION_STAGING_SRC}
)
add_custom_target(generate_version_cpp_file
    DEPENDS ${ZINC_VERSION_STAGING_SRC}
)
set_source_files_properties(${ZINC_VERSION_SRC} PROPERTIES GENERATED TRUE)
list(APPEND FILES_FOR_CLEANING ${ZINC_VERSION_SRC} ${CMAKE_CURRENT_BINARY_DIR}/configure_version_cpp_file.cmake)
list(APPEND ZINC_LIB_SRCS ${ZINC_VERSION_SRC})

#print_list_duplications( ZINC_LIB_SRCS )
#print_list_duplications( ZINC_LIB_HDRS )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/api ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/api ${INCLUDE_DIRS})

if( MSVC_IDE )
    # I want to group the source files by the directory layout for Visual Studio
    GROUP_SOURCE_FILES(${ZINC_LIB_SRCS} ${ZINC_LIB_HDRS} ${ZINC_CONFIGURE_HDRS})
    add_definitions( "/wd4244 /nologo" )
endif()

set(ZINC_LIBRARY_NAME zinc)
if( ZINC_BUILD_SHARED_LIBRARY )
    set( ZINC_SHARED_TARGET ${ZINC_LIBRARY_NAME} )
    add_library(${ZINC_SHARED_TARGET} SHARED ${ZINC_LIB_SRCS} ${ZINC_LIB_HDRS} ${ZINC_CONFIGURE_HDRS})
    list( APPEND ZINC_SHARED_DEFINITIONS -DZINC_SHARED_OBJECT )
    set( ZINC_SHARED_DEFINITIONS ${ZINC_SHARED_DEFINITIONS} PARENT_SCOPE )
    target_link_libraries(${ZINC_LIBRARY_NAME} PRIVATE ${DEPENDENT_LIBS})
    target_include_directories(${ZINC_SHARED_TARGET}
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/api>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/api>
        $<INSTALL_INTERFACE:include/>
        PRIVATE ${FREETYPE_INCLUDE_DIRS})
    if (CMAKE_VERSION VERSION_EQUAL "3.5.1" AND CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
        # No target compile features in this setting.
        target_compile_options(${ZINC_SHARED_TARGET} PRIVATE -std=c++11)
    else ()
        target_compile_features(${ZINC_SHARED_TARGET} PUBLIC cxx_auto_type)
    endif ()
    add_dependencies(${ZINC_SHARED_TARGET} generate_version_cpp_file)
    if( WIN32 )
        target_link_libraries(${ZINC_LIBRARY_NAME} PRIVATE ${LINK_PUBLIC} winmm.lib)
    endif()
    set_target_properties( ${ZINC_SHARED_TARGET} PROPERTIES
        COMPILE_DEFINITIONS "ZINC_SHARED_OBJECT"
        DEFINE_SYMBOL "ZINC_EXPORTS")
    export(TARGETS ${ZINC_SHARED_TARGET}
        FILE "${PROJECT_BINARY_DIR}/zinc-targets.cmake")
endif()

if( ZINC_BUILD_STATIC_LIBRARY )
    set(ZINC_STATIC_TARGET ${ZINC_LIBRARY_NAME}-static)
    set(ZINC_STATIC_TARGET_NAME ${ZINC_STATIC_TARGET} PARENT_SCOPE)
    add_library( ${ZINC_STATIC_TARGET} STATIC ${ZINC_LIB_SRCS} ${ZINC_LIB_HDRS} ${ZINC_CONFIGURE_HDRS} )
    target_link_libraries(${ZINC_STATIC_TARGET} PUBLIC ${DEPENDENT_LIBS})
    target_include_directories(${ZINC_STATIC_TARGET}
        PUBLIC ${FREETYPE_INCLUDE_DIRS}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/api>
        )
    add_dependencies(${ZINC_STATIC_TARGET} generate_version_cpp_file)
    if (CMAKE_VERSION VERSION_EQUAL "3.5.1" AND CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
        # No target compile features in this setting.
        target_compile_options(${ZINC_STATIC_TARGET} PRIVATE -std=c++11)
    else ()
        target_compile_features(${ZINC_STATIC_TARGET} PUBLIC cxx_auto_type)
    endif ()
    if(WIN32)
        target_link_libraries( ${ZINC_STATIC_TARGET} PUBLIC winmm.lib )
    endif()
    set_target_properties( ${ZINC_STATIC_TARGET} PROPERTIES
        OUTPUT_NAME "${ZINC_LIBRARY_NAME}" PREFIX "lib" )
    export(TARGETS ${ZINC_STATIC_TARGET}
        FILE "${PROJECT_BINARY_DIR}/zinc-targets.cmake" APPEND)
endif()

set_target_properties( ${ZINC_SHARED_TARGET} ${ZINC_STATIC_TARGET} PROPERTIES
    OUTPUT_NAME_RELEASE ${ZINC_LIBRARY_NAME}
    OUTPUT_NAME_DEBUG ${ZINC_LIBRARY_NAME}d
    VERSION ${Zinc_VERSION}
    SOVERSION ${Zinc_VERSION_MAJOR} )
# VERSION property doesn't work on Windows so we will fake it here.
if( WIN32 AND ZINC_BUILD_SHARED_LIBRARY )
    set_target_properties( ${ZINC_SHARED_TARGET} PROPERTIES
        OUTPUT_NAME_RELEASE ${ZINC_SHARED_TARGET}.${Zinc_VERSION}
        OUTPUT_NAME_DEBUG ${ZINC_SHARED_TARGET}d.${Zinc_VERSION} )
endif()

# Additional clean files
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${FILES_FOR_CLEANING}")
# Install rules for Zinc
install(TARGETS
    ${ZINC_SHARED_TARGET} ${ZINC_STATIC_TARGET}
    EXPORT ${ZINC_EXPORT_SET_NAME}
    RUNTIME DESTINATION ${ZINC_INSTALL_BIN_DIR}
    COMPONENT Runtime
    LIBRARY DESTINATION ${ZINC_INSTALL_LIB_DIR}
    COMPONENT Development
    INCLUDES DESTINATION ${ZINC_INSTALL_INC_DIR}
    ARCHIVE DESTINATION ${ZINC_INSTALL_LIB_DIR}
)

# Zinc API header install rules
install(FILES
    ${ZINC_SHARED_OBJECT_HDR}
    ${ZINC_CONFIGURE_HDR}
    ${API_HDRS}
    ${APIPP_HDRS}
    DESTINATION ${ZINC_INSTALL_INC_DIR}/opencmiss/zinc
    COMPONENT Development
)

install(FILES ${API_TYPES_HDRS} ${APIPP_TYPES_HDRS}
    DESTINATION ${ZINC_INSTALL_INC_DIR}/opencmiss/zinc/types
    COMPONENT Development
)

