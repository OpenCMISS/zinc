# OpenCMISS-Zinc Library
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(Zinc_VERSION_MAJOR @Zinc_VERSION_MAJOR@)
set(Zinc_VERSION_MINOR @Zinc_VERSION_MINOR@)
set(Zinc_VERSION_PATCH @Zinc_VERSION_PATCH@)
string(TOLOWER ${ACTIVE_BUILD_TYPE} CMAKE_BUILD_TYPE_LOWER)

set(CMAKE_MODULE_PATH @CMAKE_MODULE_PATH@)
find_package(Git QUIET)

set(ZINC_REVISION_LONG "no-revision")
if (GIT_FOUND)
    git_get_revision(ZINC_REVISION_LONG WORKING_DIRECTORY "@CMAKE_CURRENT_SOURCE_DIR@")
endif()

configure_file("@CMAKE_CURRENT_SOURCE_DIR@/configure/version.cpp.cmake"
    "@ZINC_VERSION_STAGING_SRC@" @ONLY)
