# OpenCMISS-Zinc Library
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

find_program(VIRTUALENV_EXEC virtualenv)
mark_as_advanced(VIRTUALENV_EXEC)

if (CMAKE_CONFIGURATION_TYPES)
    set(CFG_DIR /$<CONFIG>)
    set(CFG_INSTALL_DIR /\${CMAKE_INSTALL_CONFIG_NAME})
endif ()
# Bindings options
set(ZINC_VIRTUALENV_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/virtual_envs" CACHE PATH "Install prefix for the virtualenv Zinc library.")

if (NOT DEFINED ZINC_USE_VIRTUALENV)
    set(ZINC_USE_VIRTUALENV TRUE)
endif ()
set(ZINC_USE_VIRTUALENV ${ZINC_USE_VIRTUALENV} CACHE BOOL "Install Zinc into virtual environment.")

if (NOT EXISTS "${VIRTUALENV_EXEC}")
    set(ZINC_USE_VIRTUALENV FALSE CACHE BOOL "Install Zinc into virtual environment." FORCE)
    message(STATUS "The program virtualenv was not found disabling its use.")
endif ()

# Create Python bindings for the Zinc library.
set(ZINC_PYTHON_PREFERRED_VERSION "" CACHE STRING "Identify your preferred version of Python to locate, if not set the first available Python is used. [2.6, 2.7, 3.0, 3.3, ...]")

# SWIG has already been found at this point.
include(${SWIG_USE_FILE})

find_package(Python ${ZINC_PYTHON_PREFERRED_VERSION} COMPONENTS Interpreter Development QUIET)

if("${Python_VERSION_MAJOR}" STREQUAL "3")
    set(PYTHONLIBS_SWIG_FLAGS -py3)
endif()

# Find relative path between zinc library install directory and the python module dynamic libraries.
set(PYTHON_DESTINATION_PREFIX lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}${CFG_DIR}/opencmiss.zinc)
set(PYTHON_MODULE_TARGETS_DESTINATION_PREFIX "${PYTHON_DESTINATION_PREFIX}/opencmiss/zinc")
file(RELATIVE_PATH _REL_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/${PYTHON_MODULE_TARGETS_DESTINATION_PREFIX} ${CMAKE_INSTALL_PREFIX}/${ZINC_INSTALL_LIB_DIR})

# We have to set the include directories onto the directory because the swig
# wrapping process requires the include directories.
include_directories(${ZINC_API_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})

# Suppress the overload operator warning for Python.
set(CMAKE_SWIG_FLAGS -w381 -w382 -w361 -w362 ${PYTHONLIBS_SWIG_FLAGS})

set(INTERFACE_SRCS
    ../interface/context.i
    ../interface/differentialoperator.i
    ../interface/element.i
    ../interface/field.i
    ../interface/fieldmodule.i
    ../interface/fieldcache.i
    ../interface/fieldassignment.i
    ../interface/fieldparameters.i
    ../interface/fieldsmoothing.i
    ../interface/font.i
    ../interface/glyph.i
    ../interface/graphics.i
    ../interface/light.i
    ../interface/logger.i
    ../interface/material.i
    ../interface/optimisation.i
    ../interface/node.i
    ../interface/scene.i
    ../interface/scenecoordinatesystem.i
    ../interface/scenefilter.i
    ../interface/scenepicker.i
    ../interface/sceneviewer.i
    ../interface/sceneviewerinput.i
    ../interface/selection.i
    ../interface/shader.i
    ../interface/spectrum.i
    ../interface/region.i
    ../interface/result.i
    ../interface/status.i
    ../interface/stream.i
    ../interface/streamimage.i
    ../interface/streamregion.i
    ../interface/streamscene.i
    ../interface/tessellation.i
    ../interface/timekeeper.i
    ../interface/timenotifier.i
    ../interface/timesequence.i)

set_source_files_properties(${INTERFACE_SRCS} PROPERTIES CPLUSPLUS ON)

set(BASE_PYTHON_PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}${CFG_DIR}")
set(OPENCMISS_PYTHON_PACKAGE_DIR "${BASE_PYTHON_PACKAGE_DIR}/opencmiss")
set(ZINC_PYTHON_PACKAGE_DIR "${OPENCMISS_PYTHON_PACKAGE_DIR}/zinc")
set(CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/modules_python_files")

set(SWIG_MODULE_TARGETS )
set(MODULE_PY_FILES )
set(SETUP_PY_PACKAGE_FILES "'$<TARGET_FILE_NAME:zinc>'")
if (NOT WIN32)
    list(APPEND SETUP_PY_PACKAGE_FILES "'$<TARGET_SONAME_FILE_NAME:zinc>'")
    set(_GENERATOR_EXPRESSION_ZINC_SONAME $<TARGET_SONAME_FILE:zinc>)
endif ()
foreach(SWIG_INTERFACE ${INTERFACE_SRCS})
    get_filename_component(MODULE_NAME ${SWIG_INTERFACE} NAME_WE)
    list(APPEND MODULE_PY_FILES ${MODULE_NAME}.py)
    if (CMAKE_VERSION VERSION_GREATER 3.7.2)
        swig_add_library(${MODULE_NAME}
          LANGUAGE python
          SOURCES ${SWIG_INTERFACE})
    else ()
        swig_add_module(${MODULE_NAME} python ${SWIG_INTERFACE})
    endif ()
    swig_link_libraries(${MODULE_NAME} Python::Python zinc)
    set(MODULE_TARGET ${SWIG_MODULE_${MODULE_NAME}_REAL_NAME})
    # Disable use of Debug Python libraries when not present on Windows
    # See http://stackoverflow.com/questions/11311877/creating-a-dll-from-a-wrapped-cpp-file-with-swig
    if (WIN32)
        if(NOT PYTHON_DEBUG_LIBRARY)
            target_compile_definitions(${MODULE_TARGET} PRIVATE $<$<CONFIG:Debug>:SWIG_PYTHON_INTERPRETER_NO_DEBUG>)
        endif()
    endif()
    set_target_properties(${MODULE_TARGET} PROPERTIES
        DEBUG_POSTFIX ""
        LIBRARY_OUTPUT_DIRECTORY ${ZINC_PYTHON_PACKAGE_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${ZINC_PYTHON_PACKAGE_DIR}
    )
    if (APPLE)
        set_target_properties(${MODULE_TARGET} PROPERTIES
            INSTALL_RPATH "@loader_path/${_REL_INSTALL_PATH};@loader_path/")
    elseif (UNIX)
        set_target_properties(${MODULE_TARGET} PROPERTIES
            INSTALL_RPATH "\$ORIGIN/${_REL_INSTALL_PATH};\$ORIGIN/")
    endif ()
    list(APPEND SWIG_MODULE_TARGETS ${MODULE_TARGET})
    list(APPEND SETUP_PY_PACKAGE_FILES "'$<TARGET_FILE_NAME:${MODULE_TARGET}>'")
endforeach()

add_custom_target(collect_python_binding_files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SWIG_OUTDIR} ${CMAKE_CURRENT_BINARY_DIR}${CFG_DIR}/opencmiss/zinc
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:zinc> ${_GENERATOR_EXPRESSION_ZINC_SONAME} ${CMAKE_CURRENT_BINARY_DIR}${CFG_DIR}/opencmiss/zinc
)
add_dependencies(collect_python_binding_files ${SWIG_MODULE_TARGETS})

set(PYZINC_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
if(WIN32)
    # Sometimes we cross-compile on Windows so we'll set
    # the PYZINC_ARCHITECTURE manually
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(PYZINC_ARCHITECTURE x86)
    else()
        set(PYZINC_ARCHITECTURE amd64)
    endif()
endif()
if(APPLE)
    set(PYZINC_ARCHITECTURE universal)
endif()

# Get a pretty system name.
GET_SYSTEM_NAME(SYSTEM_NAME)
set(PYPI_PACKAGE_NAME opencmiss.zinc)
set(PYPI_SOURCE_TARGZ_VERSION "${Zinc_VERSION}${Zinc_DEVELOPER_VERSION}-${PYZINC_ARCHITECTURE}-${SYSTEM_NAME}-Python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")

################################################################
# Generate the required package files
set(ADDITIONAL_PACKAGE_FILES
    CHANGES.txt
    LICENSE.txt
    README.rst
    docs/main.rst)

# 1. Configure stage
set(SETUP_PY_PACKAGE_FILES_STR "${SETUP_PY_PACKAGE_FILES}")
string(REPLACE ";" "," SETUP_PY_PACKAGE_FILES_STR "${SETUP_PY_PACKAGE_FILES_STR}")
set(SETUP_PRE_GEN_PY_FILE "${CMAKE_CURRENT_BINARY_DIR}/setup.pre_gen.py")
configure_file(setup.in.py ${SETUP_PRE_GEN_PY_FILE} @ONLY)

if (WIN32)
    file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}/${ZINC_INSTALL_BIN_DIR}" NATIVE_PATH)
else()
    file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}/${ZINC_INSTALL_LIB_DIR}" NATIVE_PATH)
endif()
configure_file(zinc__init__.in.py "${CMAKE_CURRENT_BINARY_DIR}/zinc__init__.py" @ONLY)

foreach(_file ${ADDITIONAL_PACKAGE_FILES})
    configure_file(${_file} "${CMAKE_CURRENT_BINARY_DIR}/${_file}")
endforeach()
#set(MANIFEST_IN_FILE "${CMAKE_CURRENT_BINARY_DIR}/MANIFEST.in")
#configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/MANIFEST.in" "${MANIFEST_IN_FILE}")

# 2. Generate stage - takes care of all the build types (think of multiconf environment like VS)
file(GENERATE OUTPUT "${BASE_PYTHON_PACKAGE_DIR}/setup.py"
    INPUT "${SETUP_PRE_GEN_PY_FILE}"
)
# opencmiss __init__.py does not need to be configured (yet)
file(GENERATE OUTPUT "${OPENCMISS_PYTHON_PACKAGE_DIR}/__init__.py"
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/opencmiss__init__.py"
)
file(GENERATE OUTPUT "${ZINC_PYTHON_PACKAGE_DIR}/__init__.py"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/zinc__init__.py"
)
foreach(_file ${ADDITIONAL_PACKAGE_FILES})
    file(GENERATE OUTPUT "${BASE_PYTHON_PACKAGE_DIR}/${_file}"
        INPUT "${CMAKE_CURRENT_BINARY_DIR}/${_file}"
    )
endforeach()

#################################
# Create tarball
set(PYPI_SOURCE_TARGZ "${CMAKE_CURRENT_BINARY_DIR}${CFG_DIR}/${PYPI_PACKAGE_NAME}-${PYPI_SOURCE_TARGZ_VERSION}.tar.gz")
add_custom_target(python_bindings_tarball
    COMMAND ${CMAKE_COMMAND} -E tar czf ${PYPI_SOURCE_TARGZ} -- $<CONFIG>/opencmiss setup.py ${ADDITIONAL_PACKAGE_FILES}
)

include(${CMAKE_CURRENT_LIST_DIR}/InstallConfig.cmake)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()
