
# Find the PLX headers
find_path(PLX_INCLUDE_DIR
    NAMES
        PlxApi.h
    PATH
        /opt/plx/current/PlxSdk/Include)
mark_as_advanced(PLX_INCLUDE_DIR)

# Look for library
find_library(PLX_LIBRARY
    NAMES
        libPlxApi
        PlxApi
    PATHS
        ${PLX_INCLUDE_DIR}/../PlxApi/Library)
mark_as_advanced(PLX_LIBRARY)

set(PLX_VERSION_STRING "Unknown")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(plx
        REQUIRED_VARS PLX_LIBRARY PLX_INCLUDE_DIR
        VERSION_VAR PLX_VERSION_STRING)

if (NOT TARGET plx::libplx)
    add_library(plx::libplx UNKNOWN IMPORTED)
    set_target_properties(plx::libplx PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PLX_INCLUDE_DIR}")
    set_property(TARGET plx::libplx APPEND PROPERTY IMPORTED_LOCATION "${PLX_LIBRARY}")
endif()