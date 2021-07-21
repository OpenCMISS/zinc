# OpenCMISS-Zinc Library
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

FUNCTION( WXWIDGETS_HDRS XRC_SRCS _OUTFILES )

    FOREACH( XRC_SRC ${XRC_SRCS} )
        STRING( REGEX MATCH "(.*)/([a-z_A-Z]*)\\.xrc$" DUMMY "${XRC_SRC}" )
        IF( NOT EXISTS ${PROJECT_BINARY_DIR}/${CMAKE_MATCH_1} )
            FILE( MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_MATCH_1} )
        ENDIF( NOT EXISTS ${PROJECT_BINARY_DIR}/${CMAKE_MATCH_1} )
        SET( XRC_FCN ${CMAKE_MATCH_2} )
        WXWIDGETS_ADD_RESOURCES( XRC_OUTPUT ${XRC_SRC} OPTIONS --cpp-code
            --function=wxXmlInit_${XRC_FCN} --output=${PROJECT_BINARY_DIR}/${XRC_SRC}h )
    ENDFOREACH( XRC_SRC ${XRC_SRCS} )

    SET( ${_OUTFILES} ${XRC_OUTPUT} PARENT_SCOPE )
ENDFUNCTION( WXWIDGETS_HDRS _OUTFILES )

FUNCTION( TEST_FOR_VFSCANF HAVE_VFSCANF )

    INCLUDE( CheckCSourceCompiles )
    CHECK_C_SOURCE_COMPILES(
        "#include <stdio.h>
        #include <stdarg.h>

        int vread(FILE *stream, char *fmt, ...)
        {
            int rc;
            va_list arg_ptr;
            va_start(arg_ptr, fmt);
            rc = vfscanf(stream, fmt, arg_ptr);
            va_end(arg_ptr);
            return(rc);
        }

        #define MAX_LEN 80
        int main(void)
        {
            FILE *stream;
            char s[MAX_LEN + 1];
            /* Put in various data. */
            stream = fopen( \"CMakeCache.txt\", \"r\" );
            return vread(stream, \"%s\", &s[0]);
        }" VFSCANF )
            SET( ${HAVE_VFSCANF} ${VFSCANF} PARENT_SCOPE )
ENDFUNCTION( TEST_FOR_VFSCANF HAVE_VFSCANF )

FUNCTION(GROUP_SOURCE_FILES)
    FOREACH(F ${ARGV})
        STRING(REGEX REPLACE ".*/" "" SRC_RELATIVE_F ${F})
        STRING(REGEX MATCHALL "([^/]+/)" RESULT ${SRC_RELATIVE_F})
        SET(FOLDER "${RESULT}")
        LIST(LENGTH RESULT RESULT_LEN)
        IF(${RESULT_LEN} GREATER 0)
            STRING(REGEX REPLACE "/;|/" "\\\\" FOLDER_MSVC ${FOLDER})
            # If the file doesn't end in a 'c' or 'cpp' then it's a header file
            STRING(REGEX MATCH "c[p]*$" SOURCE_FILE ${F})
            IF(SOURCE_FILE)
                SET(F_LOCATION "Source Files\\${FOLDER_MSVC}")
            ELSE()
                SET(F_LOCATION "Header Files\\${FOLDER_MSVC}")
            ENDIF()
        ELSE()
            STRING(REGEX MATCH "c[p]*$" SOURCE_FILE ${F})
            IF(SOURCE_FILE)
                SET(F_LOCATION "Source Files")
            ELSE()
                SET(F_LOCATION "Header Files")
            ENDIF()
        ENDIF()
        SOURCE_GROUP(${F_LOCATION} FILES ${F})
    ENDFOREACH()
ENDFUNCTION()

FUNCTION( PRN_MSG )
    IF( DEFINED ZINC_DEBUG_PRINT )
        MESSAGE( STATUS "${ARGN}" )
    ENDIF( DEFINED ZINC_DEBUG_PRINT )
ENDFUNCTION( PRN_MSG )

FUNCTION( PRN_MSG_V )
    IF( DEFINED ZINC_DEBUG_PRINT_VERBOSE )
        MESSAGE( STATUS "${CMAKE_CURRENT_LIST_FILE}(${CMAKE_CURRENT_LIST_LINE}): ${ARGN}")
    ENDIF( DEFINED ZINC_DEBUG_PRINT_VERBOSE )
ENDFUNCTION( PRN_MSG_V )

FUNCTION( GET_SYSTEM_NAME RETURN_SYSTEM_NAME )
    IF( WIN32 )
        SET( SYSTEM_NAME "Windows" )
    ELSEIF( APPLE )
        EXECUTE_PROCESS(COMMAND sw_vers -productName RESULT_VARIABLE COMMAND_RESULT OUTPUT_VARIABLE PRODUCT_NAME )
        STRING( STRIP ${PRODUCT_NAME} PRODUCT_NAME )
        EXECUTE_PROCESS(COMMAND sw_vers -productVersion RESULT_VARIABLE COMMAND_RESULT OUTPUT_VARIABLE PRODUCT_VERSION )
        STRING( STRIP ${PRODUCT_VERSION} PRODUCT_VERSION )
        STRING( REPLACE " " "-" SYSTEM_NAME "${PRODUCT_NAME} ${PRODUCT_VERSION}" )
    ELSEIF( UNIX )
        #SET( SYSTEM_NAME "Unix-Based" )
        find_program(LSB lsb_release
            DOC "Distribution information tool")
        if (LSB)
            execute_process(COMMAND ${LSB} -i
                RESULT_VARIABLE RETFLAG
                OUTPUT_VARIABLE DISTINFO
                ERROR_VARIABLE ERRDISTINFO
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if (NOT RETFLAG)
                string(SUBSTRING ${DISTINFO} 16 -1 SYSTEM_NAME)
            endif()
        endif()
        if (NOT SYSTEM_NAME)
            EXECUTE_PROCESS(COMMAND cat /etc/issue RESULT_VARIABLE COMMAND_RESULT OUTPUT_VARIABLE COMMAND_OUTPUT ERROR_VARIABLE ERROR_OUTPUT)
            if (COMMAND_RESULT EQUAL 0)
                if (NOT COMMAND_OUTPUT MATCHES "^\\\\")
                    STRING( REGEX MATCH "^[^\\]*" COMMAND_OUTPUT ${COMMAND_OUTPUT} )
                    STRING( STRIP ${COMMAND_OUTPUT} COMMAND_OUTPUT )
                    STRING( REPLACE " " "-" SYSTEM_NAME ${COMMAND_OUTPUT} )
                endif ()
            endif ()
        endif ()
        if (NOT SYSTEM_NAME)
            EXECUTE_PROCESS(COMMAND cat /etc/os-release RESULT_VARIABLE COMMAND_RESULT OUTPUT_VARIABLE COMMAND_OUTPUT ERROR_VARIABLE ERROR_OUTPUT)
            if (COMMAND_RESULT EQUAL 0)
                string(REGEX MATCH "^NAME=\\\"([^\\\"]*)" NAME_OUTPUT ${COMMAND_OUTPUT})
                string(REPLACE " " "-" SYSTEM_NAME ${CMAKE_MATCH_1})
            endif ()
        endif ()
        if (NOT SYSTEM_NAME)
            set(SYSTEM_NAME Unix-Based)
        endif ()
    ELSE()
        MESSAGE( FATAL_ERROR "Don't yet know this system." )
    ENDIF()

    STRING(REPLACE "\n" "_" SYSTEM_NAME ${SYSTEM_NAME})

    SET( ${RETURN_SYSTEM_NAME} ${SYSTEM_NAME} PARENT_SCOPE)
ENDFUNCTION()

