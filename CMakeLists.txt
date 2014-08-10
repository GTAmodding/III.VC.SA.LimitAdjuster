project(limit_adjuster)
cmake_minimum_required(VERSION 2.8)

# When no build type specified, use Release
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
    message("Using default build type that is Release")
endif()


set(DOC_DIRECTORY "${CMAKE_SOURCE_DIR}/doc")
set(CONFIGS_DIR "${DOC_DIRECTORY}")

# Output directories
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(MAKE_SHARED_MODULE_PREFIX "")
set(CMAKE_STATIC_LIBRARY_PREFIX "")

# Ok, run source directory
add_subdirectory(src)

# ----------------------------> Install
set(CMAKE_INSTALL_PREFIX "")

# Install config files
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Don't copy configs in Debug
else()
    install(FILES "${CONFIGS_DIR}/limit_adjuster_gta3vcsa.ini" DESTINATION "./")
endif()
 
 