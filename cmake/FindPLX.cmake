
# Find the PLX headers
find_path(PLX_INCLUDE_DIR
    NAMES
        Include/PlxApi.h
    PATH
        /opt/plx/current/PlxSdk)
mark_as_advanced(PLX_INCLUDE_DIR)

message(STATUS  "Found: " ${PLX_INCLUDE_DIR})

# Look for library
find_library(PLX_LIBRARY
    NAMES
        PlxApi/Library/libPlxApi
        PlxApi/Library/PlxApi
    PATHS
        /opt/plx/current/PlxSdk)
mark_as_advanced(PLX_LIBRARY)

set(PLX_VERSION_STRING "Unknown")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PLX
        REQUIRED_VARS PLX_LIBRARY PLX_INCLUDE_DIR
        VERSION_VAR PLX_VERSION_STRING)

if (NOT TARGET plx::libplx)
    add_library(plx::libplx UNKNOWN IMPORTED)
    set_target_properties(plx::libplx PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PLX_INCLUDE_DIR}")
    set_property(TARGET plx::libplx APPEND PROPERTY IMPORTED_LOCATION "${PLX_LIBRARY}")
endif()