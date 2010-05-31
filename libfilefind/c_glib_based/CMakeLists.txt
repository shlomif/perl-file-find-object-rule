SET(DEBUG "" CACHE BOOL "Add debugging information")
# This is the equivalent to perform a "make dist"/"make distdir" etc.
SET(CPACK_PACKAGE_NAME "libfilefind")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A C Library for Traversing a Directory Recursively")
SET(CPACK_PACKAGE_VENDOR "Shlomi Fish")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")

# Process and extract the version number.
FILE( READ "ver.txt" VERSION)

CHOMP (VERSION)

STRING (REGEX MATCHALL "([0-9]+)" VERSION_DIGITS "${VERSION}")

LIST(GET VERSION_DIGITS 0 CPACK_PACKAGE_VERSION_MAJOR)
LIST(GET VERSION_DIGITS 1 CPACK_PACKAGE_VERSION_MINOR)
LIST(GET VERSION_DIGITS 2 CPACK_PACKAGE_VERSION_PATCH)

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
    "/tags$"
    "/config\\\\.h$"
    "/Testing/"
    "/test_rwlock$"
    "/test_rwlock_2$"
)

IF(WIN32 AND NOT UNIX)
    # There is a bug in NSI that does not handle full unix paths properly. Make
    # sure there is at least one set of four (4) backlasshes.
    SET(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\dondorf-king.bmp")
    SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\fc-solve.exe")
    SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\fc-solve.berlios.de")
    SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\fc-solve.berlios.de")
    SET(CPACK_NSIS_DISPLAY_NAME "Freecell Solver")
    SET(CPACK_NSIS_CONTACT "shlomif@iglu.org.il")
    SET(CPACK_NSIS_MODIFY_PATH ON)
    # Setting for NSIS :
    SET(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\fc-solve.ico")
    SET(CPACK_NSIS_MUI_UNIICON ${CPACK_NSIS_MUI_ICON})
    SET(CPACK_PACKAGE_ICON ${CPACK_NSIS_MUI_ICON})
    SET(CPACK_NSIS_MODIFY_PATH "ON")
ELSE(WIN32 AND NOT UNIX)
ENDIF(WIN32 AND NOT UNIX)

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

ADD_SUBDIRECTORY ("man")

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET (CFLAG_TO_CHECK "-Wall")
CHECK_C_COMPILER_FLAG(${CFLAG_TO_CHECK} CFLAG_GCC_ALL_WARNS)
IF (${CFLAG_GCC_ALL_WARNS})
    ADD_DEFINITIONS(${CFLAG_TO_CHECK})
ENDIF (${CFLAG_GCC_ALL_WARNS})

IF (${DEBUG})
    ADD_DEFINITIONS("-DFILEFIND_DEBUG=1")
ENDIF (${DEBUG})

# So it can find config.h
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

SET (FILEFIND_MODULES file_find.c)

ADD_LIBRARY (libfilefind
    SHARED
    ${FILEFIND_MODULES}
    )

LIST (APPEND PTHREAD_RWLOCK_FCFS_LIBS "libfilefind")
IF (${BUILD_STATIC_LIBRARY})
    ADD_LIBRARY (libfilefind-static
        STATIC
        ${FILEFIND_MODULES}
    )
    LIST (APPEND PTHREAD_RWLOCK_FCFS_LIBS "libfilefind-static")

    SET_TARGET_PROPERTIES(
        libfilefind-static
        PROPERTIES OUTPUT_NAME "libfilefind"
    )
    SET_TARGET_PROPERTIES(
        ${PTHREAD_RWLOCK_FCFS_LIBS}
        PROPERTIES CLEAN_DIRECT_OUTPUT 1
    )
ENDIF (${BUILD_STATIC_LIBRARY})

SET_TARGET_PROPERTIES(libfilefind
    PROPERTIES VERSION 0.1.0 SOVERSION 0
    )

TARGET_LINK_LIBRARIES (libfilefind pthread)

ADD_EXECUTABLE (test_rwlock
    test_rwlock.c
)

TARGET_LINK_LIBRARIES (test_rwlock libfilefind)

ADD_EXECUTABLE (test_rwlock_2
    test_rwlock_2.c
)

TARGET_LINK_LIBRARIES (test_rwlock_2 libfilefind)

INSTALL(
    FILES
        "AUTHORS"
        "COPYING"
        "NEWS"
        "README"
        "TODO"
    DESTINATION
        "share/doc/libfilefind/"
)

INSTALL(
    FILES
        "libfilefind/filefind.h"
    DESTINATION
        "include/pthread"
)

ENABLE_TESTING()

ADD_TEST(
    runtime
    ./test_rwlock --timeout 5
)

ADD_TEST(
    runtime2
    ./test_rwlock_2 --timeout 5
)