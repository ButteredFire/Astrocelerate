# This file helps find the SPICE C library (cspice)

# SPICE_ROOT_DIR can be set to specify the installation location
if (NOT SPICE_ROOT_DIR)
    set(SPICE_ROOT_DIR "$ENV{SPICE_ROOT_DIR}")
endif()


# Find the header files and compiled library file
find_path(SPICE_INCLUDE_DIR
    NAMES SpiceUsr.h
    HINTS ${SPICE_ROOT_DIR}/include
)


find_library(SPICE_LIBRARY
    NAMES cspice libcspice  # cspice on Windows, libcspice on Linux/MacOS
    HINTS ${SPICE_ROOT_DIR}/lib
)


# Handle the result
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SPICE
    DEFAULT_MSG
    SPICE_LIBRARY SPICE_INCLUDE_DIR
)


# Set SPICE_FOUND
if (SPICE_FOUND)
    set(SPICE_LIBRARIES ${SPICE_LIBRARY})
    set(SPICE_INCLUDE_DIRS ${SPICE_INCLUDE_DIR})
endif()

mark_as_advanced(SPICE_ROOT_DIR SPICE_INCLUDE_DIR SPICE_LIBRARY)