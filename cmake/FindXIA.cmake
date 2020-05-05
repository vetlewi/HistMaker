
# Find the PLX headers
find_path(XIA_INCLUDE_DIR
    NAMES
        app/pixie16app_export.h
        sys/pixie16sys_export.h
        inc/PlxApi.h
    PATH
        /opt/xia/current)
mark_as_advanced(XIA_INCLUDE_DIR)

# Look for library
find_library(XIA_LIBRARY
    NAMES
        libPixie16App
        libPixie16Sys
        Pixie16App
        Pixie16Sys
    PATHS
        ${XIA_INCLUDE_DIR})
mark_as_advanced(XIA_LIBRARY)

set(XIA_VERSION_STRING "Unknown")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(xia
        REQUIRED_VARS XIA_LIBRARY XIA_INCLUDE_DIR
        VERSION_VAR XIA_VERSION_STRING)

if (NOT TARGET xia::libpixie)
    add_library(xia::libpixie UNKNOWN IMPORTED)
    set_target_properties(xia::libpixie PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${XIA_INCLUDE_DIR}")
    set_property(TARGET xia::libpixie APPEND PROPERTY IMPORTED_LOCATION "${XIA_LIBRARY}")
endif()