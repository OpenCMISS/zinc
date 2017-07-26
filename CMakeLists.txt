
# OpenCMISS-Zinc Library
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.0)

# This is the project name and shows up in IDEs
project(Zinc VERSION 3.1.1 LANGUAGES C CXX)

option(ZINC_BUILD_TESTS "${PROJECT_NAME} - Build tests." ON)
option(ZINC_BUILD_BINDINGS "Build bindings for ${PROJECT_NAME}, requires SWIG." YES)
option(ZINC_BUILD_SHARED_LIBRARY "Build a shared zinc library." ON)
option(ZINC_BUILD_STATIC_LIBRARY "Build a static zinc library." OFF)
option(ZINC_PRINT_CONFIG_SUMMARY "Show a summary of the configuration." TRUE)

# Check bare minimum configuration
set(ZINC_CMAKE_MODULE_PATH "" CACHE STRING "Location of OpenCMISS CMake modules.")
if (DEFINED OPENCMISS_CMAKE_MODULE_PATH)
    set(ZINC_CMAKE_MODULE_PATH "${OPENCMISS_CMAKE_MODULE_PATH}" CACHE STRING "Location of OpenCMISS CMake modules." FORCE)
endif ()
if (NOT EXISTS "${ZINC_CMAKE_MODULE_PATH}/Modules/OpenCMISS/OCMiscFunctions.cmake")
    message(FATAL_ERROR "'${ZINC_CMAKE_MODULE_PATH}/Modules/OpenCMISS/OCMiscFunctions.cmake' does not exists.  The OpenCMISS CMake modules may not be installed/available.")
endif ()

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${ZINC_CMAKE_MODULE_PATH}/FindModuleWrappers")
list(APPEND CMAKE_MODULE_PATH "${ZINC_CMAKE_MODULE_PATH}/Modules")
list(APPEND CMAKE_MODULE_PATH "${ZINC_CMAKE_MODULE_PATH}/Modules/OpenCMISS")

set(ZINC_PACKAGE_CONFIG_DIR "lib/cmake" CACHE STRING "Path for CMake package configuration files (relative to CMAKE_INSTALL_PREFIX or an absolute path).")
if (DEFINED OPENCMISS_PACKAGE_CONFIG_DIR)
    set(ZINC_PACKAGE_CONFIG_DIR "${OPENCMISS_PACKAGE_CONFIG_DIR}" CACHE STRING "Path for CMake package configuration files (relative to CMAKE_INSTALL_PREFIX or an absolute path)." FORCE)
endif ()

if(ZINC_BUILD_BINDINGS)
    option(ZINC_BUILD_PYTHON_BINDINGS "Build Python bindings for ${PROJECT_NAME}" YES)
endif()

include(OCMultiConfigEnvironment)

# Optional libraries
option(ZINC_USE_IMAGEMAGICK "Use ImageMagick" YES)
if (NOT ZINC_USE_IMAGEMAGICK)
    option(ZINC_USE_PNG "Use png" YES)
endif()
option(ZINC_USE_NETGEN "Use Netgen" YES)
option(ZINC_USE_ITK "Use ITK" YES) # Not really an option until image fields are worked on

# The name for the project seen in directory paths
set(ZINC_BASE_NAME zinc)

if(NOT ZINC_BUILD_SHARED_LIBRARY AND NOT ZINC_BUILD_STATIC_LIBRARY )
    message(FATAL_ERROR "You have to build at least one of the types of libraries (shared or static)." )
endif()

include(MacroDefinitions)
include(FunctionDefinitions)
include(OCMiscFunctions)

find_package(Git QUIET)
find_package(SWIG QUIET)

set(ZINC_REVISION "no-rev")
if (GIT_FOUND)
    git_get_revision(ZINC_REVISION_LONG)
    string(SUBSTRING ${ZINC_REVISION_LONG} 0 6 ZINC_REVISION)
endif()

set(ZINC_RELEASE FALSE)
if(ZINC_RELEASE)
    set(ZINC_DEVELOPER_VERSION "")
else()
    set(ZINC_DEVELOPER_VERSION ".r${ZINC_REVISION}")
endif()

# Make relative paths absolute (needed later on)
foreach( p LIB BIN INC CMAKE )
    set(var ZINC_INSTALL_${p}_DIR )
    if(NOT IS_ABSOLUTE "${${var}}")
        set(${var}_ABSOLUTE "${CMAKE_INSTALL_PREFIX}/${${var}}" )
    endif()
endforeach()

# Must have dependencies
find_package(ZLIB ${ZLIB_VERSION} REQUIRED)
find_package(BZip2 ${BZIP2_VERSION} REQUIRED)
find_package(LibXml2 ${LIBXML2_VERSION} REQUIRED)
find_package(Fieldml-API ${FIELDML-API_VERSION} REQUIRED)
find_package(FTGL ${FTGL_VERSION} REQUIRED)
find_package(Freetype REQUIRED)
#include_directories(${FREETYPE_INCLUDE_DIRS})
find_package(OPTPP ${OPTPP_VERSION} REQUIRED)
find_package(GLEW ${GLEW_VERSION} REQUIRED)
set(USE_GLEW TRUE)
if(WIN32)
    set(GLEW_STATIC TRUE)
endif()
set(DEPENDENT_LIBS zlib bz2 xml2 fieldml-core fieldml-io ftgl optpp glew)
set(ZINC_DEPS ZLIB BZip2 LibXml2 Fieldml-API FTGL OPTPP GLEW)

set(USE_MSAA TRUE)

# Define variables to false that need to be at least defined for passsing to option_with_default macro.
# Use options
if(${CMAKE_BUILD_TYPE} MATCHES "[Dd]ebug" )
    option(ZINC_BUILD_MEMORYCHECK "Do you want to build a memory check version" OFF )
endif()

# Set graphics
if(NOT DEFINED GRAPHICS_API )
    set(GRAPHICS_API OPENGL_GRAPHICS )
endif()

# Set build type dependent #Defines
if(${CMAKE_GENERATOR} MATCHES "Visual Studio" )
    set(CREATING_FOR_MULTI_BUILD_TYPE TRUE )
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "[Dd]ebug" )
    set(USE_PARAMETER_ON TRUE )
    if(NOT ${COMPILER_NAME} MATCHES "msvc" )
        set(REPORT_GL_ERRORS )
    endif()
else()
    if(CREATING_FOR_MULTI_BUILD_TYPE )
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DOPTIMISED" )
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DOPTIMISED" )
    else()
        set(OPTIMISED TRUE )
    endif()
endif()

if(ZINC_BUILD_MEMORYCHECK )
    set(MEMORY_CHECKING TRUE )
endif()

# Set the configurable typedefs
set(ZnReal double )
set(FE_value ${ZnReal} )
set(FE_VALUE_INPUT_STRING \"%lf\" )
set(FE_VALUE_STRING \"22.15le\" )
set(COLOUR_PRECISION ${ZnReal} )
set(MATERIAL_PRECISION ${ZnReal} )

if(APPLE)
    find_library(ICONV_LIBRARY NAMES iconv)
    if(ICONV_LIBRARY )
        list(APPEND DEPENDENT_LIBS ${ICONV_LIBRARY})
    endif()
endif()

# Set Target name and global defines
SET_PLATFORM_DEFINES( )
SET_OPERATING_SYSTEM_DEFINES( ) # <-- mostly set automatically by cmake just adding WIN32_SYSTEM to be used instead of WIN32
SET_GRAPHICS_DEFINES( )

#netgen
if(ZINC_USE_NETGEN)
    find_package(NETGEN ${NETGEN_VERSION} REQUIRED)
    list(APPEND DEPENDENT_LIBS nglib)
    list(APPEND ZINC_DEPS NETGEN)
endif()

if(ZINC_USE_PNG)
    find_package(PNG ${PNG_VERSION} CONFIG REQUIRED)
    list(APPEND DEPENDENT_LIBS png)
    list(APPEND ZINC_DEPS PNG)
endif()

if(ZINC_USE_IMAGEMAGICK)
    find_package(ImageMagick ${IMAGEMAGICK_VERSION} CONFIG REQUIRED)
    list(APPEND DEPENDENT_LIBS MagickCore)
    list(APPEND ZINC_DEPS ImageMagick)
endif()

if(ZINC_USE_ITK )
    find_package(ITK ${ITK_VERSION} CONFIG REQUIRED)
    list(APPEND DEPENDENT_LIBS  ${ITK_LIBRARIES})
    list(APPEND ZINC_DEPS ITK)
    include_directories(${ITK_INCLUDE_DIRS})
endif()

#if(ZINC_USE_OCE )
#	if(OCE_FOUND )
#		MARK_AS_ADVANCED( OCE_DIR )
#		set(USE_OPENCASCADE TRUE )
#		set(DEPENDENT_DEFINITIONS ${DEPENDENT_DEFINITIONS} USE_CAD_FIELDS HAVE_CONFIG_H )
#		set(DEPENDENT_LIBS ${DEPENDENT_LIBS} ${OCE_LIBRARIES} )
#		set(INCLUDE_DIRS ${INCLUDE_DIRS} ${OCE_INCLUDE_DIRS} )
#	else( OCE_FOUND )
#		message(FATAL_ERROR "OCE was requested but not found." )
#	endif(OCE_FOUND )
#endif(ZINC_USE_OCE )

# OpenCASCADE - is a config file therefore I cannot find twice
#if(ZINC_USE_OPENCASCADE )
#	if(OPENCASCADE_FOUND )
		# Subset of OpenCASCADE libraries Zinc is dependent on
		# TKernel, TKMath, TKAdvTools, TKGeomBase, TKGeomAlgo, TKBRep, TKPrim,
		# TKBO, TKBool, TKG2d, TKG3d, PTKernel, TKOffset, TKFillet, TKMESH, TKIGES,
		# TKXCAF, TKCAF, TKV3d, TKLCAF, TKTopAlgo, TKCDF, TKV2d, TKHLR, TKService,
		# TKShHealing, TKXSBase, TKSTEP, TKSTEPBase, TKSTEPAttr, TKSTEP209,
		# TKXDEIGES, TKXDESTEP
#		mark_as_advanced( OpenCASCADE_DIR )
#		set(USE_OPENCASCADE TRUE )
#		set(OPENCASCADE_DEFINITIONS USE_CAD_FIELDS )
#		if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" )
			# 64-bit architecture
#			set(OPENCASCADE_DEFINITIONS ${OPENCASCADE_DEFINITIONS} _OCC64 )
#			set(OPENCASCADE_CXX_FLAGS -m64 )
#		endif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" )
#		if(WIN32 )
#			set(OPENCASCADE_DEFINITIONS ${OPENCASCADE_DEFINITIONS} WNT HAVE_NO_DLL )
#		else( WIN32 )
#			set(OPENCASCADE_DEFINITIONS ${OPENCASCADE_DEFINITIONS} HAVE_OCC_CONFIG_H )
#		endif(WIN32 )
#		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPENCASCADE_CXX_FLAGS}" )
#
#		set(DEPENDENT_LIBS ${DEPENDENT_LIBS} ${OPENCASCADE_LIBRARIES} )
#		set(INCLUDE_DIRS ${INCLUDE_DIRS} ${OPENCASCADE_INCLUDE_DIRS} )
#		set(DEPENDENT_CONFIGS "${DEPENDENT_CONFIGS}\nINCLUDE(${OpenCASCADE_DIR}/opencascade-config.cmake)" )
#		set(DEPENDENT_DEFINITIONS ${DEPENDENT_DEFINITIONS} ${OPENCASCADE_DEFINITIONS} )
#	else( OPENCASCADE_FOUND )
#		message(FATAL_ERROR "OpenCASCADE was requested but not found." )
#	endif(OPENCASCADE_FOUND )
#endif(ZINC_USE_OPENCASCADE )

if(${GRAPHICS_API} MATCHES OPENGL_GRAPHICS )
    find_package(OpenGL REQUIRED )
    set(DEPENDENT_LIBS ${DEPENDENT_LIBS} ${OPENGL_LIBRARIES} )
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -Wall -fvisibility=hidden -Wl,--as-needed" )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Wall -fvisibility=hidden -Wl,--as-needed" )
	set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} --as-needed" )
	if(${CMAKE_BUILD_TYPE} MATCHES [Rr]elease )
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter" )
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter" )
	endif(${CMAKE_BUILD_TYPE} MATCHES [Rr]elease )
elseif(MSVC)
	set(PLATFORM_COMPILER_DEFINITIONS ${PLATFORM_COMPILER_DEFINITIONS} _CRT_SECURE_NO_WARNINGS _CRTDBG_MAP_ALLOC)
endif()

TEST_FOR_VFSCANF( HAVE_VFSCANF )
include( CheckFunctionExists )
CHECK_FUNCTION_EXISTS( heapsort HAVE_HEAPSORT )

foreach(DEF ${DEPENDENT_DEFINITIONS} ${PLATFORM_COMPILER_DEFINITIONS})
	add_definitions(-D${DEF})
endforeach()
set(ZINC_STATIC_DEFINITIONS )
foreach( DEF ${DEPENDENT_DEFINITIONS} )
	LIST( APPEND ZINC_STATIC_DEFINITIONS -D${DEF} )
endforeach()
set(ZINC_SHARED_DEFINITIONS )

set(ZINC_API_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/core/source/api;${PROJECT_BINARY_DIR}/core/source/api")
#include_directories(${PROJECT_BINARY_DIR}/core/source/api)

# Set those before we define libraries in subfolders
set(ZINC_INSTALL_LIB_DIR lib)# "Installation directory for libraries"
set(ZINC_INSTALL_BIN_DIR bin)# "Installation directory for executables"
set(ZINC_INSTALL_INC_DIR include)# "Installation directory for header files"
set(ZINC_EXPORT_SET_NAME zinc-config)

add_subdirectory(core) # Defines ZINC_LIBRARY_TARGETS

if(ZINC_BUILD_TESTS)
    enable_testing()

    # Set up the correct environment for the tests
    # See e.g. https://cmake.org/pipermail/cmake/2009-May/029464.html
    if (WIN32)
       set(LD_VARNAME "PATH")
    else()
       set(LD_VARNAME "LD_LIBRARY_PATH")
    endif()

    add_subdirectory(tests)
endif()

if(SWIG_FOUND AND ZINC_BUILD_BINDINGS)
    add_subdirectory(bindings)
endif()

string(REGEX REPLACE ";" " " INCLUDE_DIRS "${INCLUDE_DIRS}" )
string(REGEX REPLACE ";" " " DEPENDENT_DEFINITIONS "${DEPENDENT_DEFINITIONS}")

# Create the build tree imported targets
export(TARGETS ${ZINC_SHARED_TARGET} ${ZINC_STATIC_TARGET}
	FILE "${PROJECT_BINARY_DIR}/ZincLibraryDepends.cmake")

# Register the Zinc project with the CMake database
export(PACKAGE ${ZINC_BASE_NAME})

# Install ZincLibraryDepends.cmake to the install destination
install(EXPORT ${ZINC_EXPORT_SET_NAME}
	DESTINATION ${ZINC_PACKAGE_CONFIG_DIR}
	COMPONENT Development
)
include(CMakePackageConfigHelpers)
WRITE_BASIC_PACKAGE_VERSION_FILE(${CMAKE_CURRENT_BINARY_DIR}/${ZINC_EXPORT_SET_NAME}-version.cmake
	COMPATIBILITY AnyNewerVersion)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${ZINC_EXPORT_SET_NAME}-version.cmake
        DESTINATION ${ZINC_PACKAGE_CONFIG_DIR}
        COMPONENT Development
)
set(DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/${ZINC_EXPORT_SET_NAME}-dependencies.cmake)
file(WRITE ${DEPFILE}
	"include(CMakeFindDependencyMacro)\n"
	"set(${PROJECT_NAME}_IMPORT_PREFIX \${_IMPORT_PREFIX})\n"
)
foreach(ZINCDEP ${ZINC_DEPS})
    if (${ZINCDEP}_FOUND)
        file(APPEND ${DEPFILE} "find_dependency(${ZINCDEP})\n")
	else ()
        message(STATUS "Requested dependency '${ZINCDEP}' not found!  Therefore not adding to list of dependencies!")
    endif ()
endforeach()
file(APPEND ${DEPFILE} "set(_IMPORT_PREFIX \${${PROJECT_NAME}_IMPORT_PREFIX})\n")
install(FILES
        ${DEPFILE}
        DESTINATION ${ZINC_PACKAGE_CONFIG_DIR}
        COMPONENT DevelopmentSDK
)

# Packaging for the Zinc library
include(cmake/ZincPackaging.cmake)

# Output configuration information if requested
if(ZINC_BUILD_SHARED_LIBRARY )
	set(ZINC_BUILD_STRING "shared")
endif()
if(ZINC_BUILD_STATIC_LIBRARY )
	set(ZINC_BUILD_STRING "${ZINC_BUILD_STRING} static")
endif()

if(ZINC_PRINT_CONFIG_SUMMARY)
	message(STATUS "")
	message(STATUS "=====================================================")
	message(STATUS "Zinc has been configured with the following settings:")
	message(STATUS "")
	message(STATUS "     Source revision: '${ZINC_REVISION}'")
	message(STATUS "         Target type: '${ZINC_BUILD_STRING}'")
	message(STATUS "          Build type: '${CMAKE_BUILD_TYPE}'")
	message(STATUS "         Install dir: '${CMAKE_INSTALL_PREFIX}'")
	if (VIRTUALENV_LOCATION AND ZINC_BUILD_PYTHON_BINDINGS)
		message(STATUS "   Python VirtualEnv: '${VIRTUALENV_LOCATION}'")
	endif()
	foreach(ZINCDEP ${ZINC_DEPS})
		if (${ZINCDEP}_FOUND)
			string(SUBSTRING "${ZINCDEP}              " 0 11 ZINCDEP_FIXED_SIZE)
			message(STATUS "         ${ZINCDEP_FIXED_SIZE}: YES")
		endif()
	endforeach()
	foreach(DEPLIB ${DEPENDENT_LIBS})
		if (TARGET ${DEPLIB})
			get_target_property(TARGET_TYPE ${DEPLIB} TYPE)
			if (TARGET_TYPE STREQUAL INTERFACE_LIBRARY)
				get_target_property(TARGET_ILR ${DEPLIB} INTERFACE_LINK_LIBRARIES)
				set(TARGET_ILR "Interface to ${TARGET_ILR}")
			else()
				get_target_property(TARGET_ILR ${DEPLIB} IMPORTED_LOCATION_RELEASE)
				if (NOT TARGET_ILR)
					get_target_property(TARGET_ILR ${DEPLIB} IMPORTED_LOCATION_DEBUG)
				endif()
			endif()
			string(SUBSTRING "${DEPLIB}              " 0 11 DEPLIB_FIXED_SIZE)
			message(STATUS "         ${DEPLIB_FIXED_SIZE}: '${TARGET_ILR}'")
		endif()
	endforeach()
	message(STATUS "=====================================================")
	message(STATUS "")
endif()



