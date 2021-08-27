CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT( "libfilefind" )

INCLUDE ("${CMAKE_SOURCE_DIR}/cmake/shlomif_common_bootstrap.cmake")
SHLOMIF_COMMON_SETUP("${private_mod_path}")
INCLUDE(GNUInstallDirs)

INCLUDE(FindPkgConfig)

SET(MYNAME "filefind")
SET(DEBUG "" CACHE BOOL "Add debugging information")
# This is the equivalent to perform a "make dist"/"make distdir" etc.
SET(CPACK_PACKAGE_NAME "lib${MYNAME}")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A C Library for Traversing a Directory Recursively")
SET(CPACK_PACKAGE_VENDOR "Shlomi Fish")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")

# Introduces VERSION , CPACK_PACKAGE_VERSION_MAJOR,
# CPACK_PACKAGE_VERSION_MAJOR, and CPACK_PACKAGE_VERSION_PATCH
READ_VERSION_FROM_VER_TXT()

SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY} ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

SET(CPACK_SOURCE_IGNORE_FILES
    "/_CPack_Packages/"
    "/CMakeFiles/"
    "/.deps/"
    "libfilefind-[0-9]+\\\\.[0-9]+\\\\.[0-9]+(-Source|-Linux|)(/|\\\\.(sh|tar\\\\.(gz|bz2|Z|lzma|xz))$)"
    "\\\\.o$"
    "~$"
    "/lib(filefind)\\\\.a$"
    "\\\\.so(\\\\.[0-9]+)*$"
    "/.svn/"
    "/CMakeCache.txt$"
    "/Makefile$"
    "/CTestTestfile.cmake$"
    "/cmake_install.cmake$"
    "/CPackConfig.cmake$"
    "/CPackSourceConfig.cmake$"
    "/minifind$"
    "/tags$"
    "/config\\\\.h$"
    "/Testing/"
    "/test_rwlock$"
    "/test_rwlock_2$"
)

IF (WIN32 AND NOT UNIX)
    # There is a bug in NSI that does not handle full unix paths properly. Make
    # sure there is at least one set of four (4) backlasshes.
    SET(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\dondorf-king.bmp")
    SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\fc-solve.exe")
    SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\fc-solve.berlios.de")
    SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\fc-solve.berlios.de")
    SET(CPACK_NSIS_DISPLAY_NAME "Freecell Solver")
    SET(CPACK_NSIS_CONTACT "shlomif@cpan.org")
    SET(CPACK_NSIS_MODIFY_PATH ON)
    # Setting for NSIS :
    SET(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\fc-solve.ico")
    SET(CPACK_NSIS_MUI_UNIICON ${CPACK_NSIS_MUI_ICON})
    SET(CPACK_PACKAGE_ICON ${CPACK_NSIS_MUI_ICON})
    SET(CPACK_NSIS_MODIFY_PATH "ON")
ENDIF ()

### This is to set the RPATH correctly, so when installed under a prefix
### the executables will find the libraries.
###
### See:
###
### http://www.cmake.org/Wiki/CMake_RPATH_handling
###
### (Taken from that wiki page)

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# the RPATH to be used when installing
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

INCLUDE(CPack)

include(CheckFunctionExists)
INCLUDE(CheckCCompilerFlag)

SET (CFLAG_TO_CHECK "-Wall")
CHECK_C_COMPILER_FLAG(${CFLAG_TO_CHECK} CFLAG_GCC_ALL_WARNS)
IF (${CFLAG_GCC_ALL_WARNS})
    ADD_DEFINITIONS(${CFLAG_TO_CHECK})
ENDIF ()

IF (${DEBUG})
    ADD_DEFINITIONS("-DFILEFIND_DEBUG=1")
ENDIF ()

# Inspired from /usr/share/autoconf/autoconf/c.m4
FOREACH(KEYWORD "inline" "__inline__" "__inline")
    IF(NOT DEFINED HAVE_C_INLINE)
        TRY_COMPILE(C_HAS_${KEYWORD} "${CMAKE_CURRENT_BINARY_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/test_inline.c"
            COMPILE_DEFINITIONS "-Dinline=${KEYWORD}")
        IF(C_HAS_${KEYWORD})
            SET(HAVE_C_INLINE TRUE)
            SET(FCS_INLINE_KEYWORD "${KEYWORD}")
        ENDIF ()
    ENDIF ()
ENDFOREACH ()


CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# So it can find config.h
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

SET (FILEFIND_MODULES filefind.c)
# PKG_CHECK_MODULES (GLIB2 REQUIRED glib-2.0)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)

ADD_DEFINITIONS(${GLIB2_CFLAGS})

SET (LIBNAME "${MYNAME}")
ADD_LIBRARY ("${LIBNAME}"
    SHARED
    ${FILEFIND_MODULES}
)

target_link_libraries( "${LIBNAME}" PkgConfig::deps)
# SET_TARGET_PROPERTIES( "${LIBNAME}" PROPERTIES LINK_FLAGS ${GLIB2_LDFLAGS})

LIST (APPEND PTHREAD_RWLOCK_FCFS_LIBS "${LIBNAME}")
IF (${BUILD_STATIC_LIBRARY})
    SET (STAT_NAME "${MYNAME}-static")
    ADD_LIBRARY ("${STAT_NAME}"
        STATIC
        ${FILEFIND_MODULES}
    )
    LIST (APPEND PTHREAD_RWLOCK_FCFS_LIBS "${STAT_NAME}")

    SET_TARGET_PROPERTIES(
        "${STAT_NAME}"
        PROPERTIES OUTPUT_NAME "${LIBNAME}"
    )
    SET_TARGET_PROPERTIES(
        ${PTHREAD_RWLOCK_FCFS_LIBS}
        PROPERTIES CLEAN_DIRECT_OUTPUT 1
    )
ENDIF ()

SET_TARGET_PROPERTIES("${LIBNAME}"
    PROPERTIES VERSION 0.1.0 SOVERSION 0
)

ADD_EXECUTABLE("minifind" "minifind.c")
TARGET_LINK_LIBRARIES("minifind" "${LIBNAME}")
# SET_TARGET_PROPERTIES("minifind" PROPERTIES LINK_FLAGS ${GLIB2_LDFLAGS})

INSTALL(
    FILES
        "AUTHORS"
        "COPYING"
        "NEWS"
        "README"
        "TODO"
    DESTINATION
        "share/doc/${CPACK_PACKAGE_NAME}/"
)

INSTALL(
    FILES
        "${LIBNAME}/filefind.h"
    DESTINATION
        "include/pthread"
)

ADD_CUSTOM_TARGET(
    "test"
    "perl" "${CMAKE_CURRENT_SOURCE_DIR}/run-tests.pl"
)
