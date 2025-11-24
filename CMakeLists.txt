cmake_minimum_required(VERSION 3.19)

project(mc-afker VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directory for binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Configure Botcraft options
# Disable building examples since we're building our own project
set(BOTCRAFT_BUILD_EXAMPLES OFF CACHE BOOL "Build Botcraft examples" FORCE)
# Disable tests by default
set(BOTCRAFT_BUILD_TESTS OFF CACHE BOOL "Build Botcraft tests" FORCE)
# Enable compression and encryption by default
set(BOTCRAFT_COMPRESSION ON CACHE BOOL "Enable compression" FORCE)
set(BOTCRAFT_ENCRYPTION ON CACHE BOOL "Enable encryption" FORCE)

# Add Botcraft library as subdirectory
add_subdirectory(libs/Botcraft)

# Source files
set(MC_AFKER_SOURCES
    src/main.cpp
)

# Create executable
add_executable(mc-afker ${MC_AFKER_SOURCES})

# Link against botcraft library
# Use PUBLIC to ensure include directories are propagated
target_link_libraries(mc-afker PUBLIC botcraft)

# Set C++ standard
set_target_properties(mc-afker PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Configure output directories
if(MSVC)
    set_target_properties(mc-afker PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin"
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
    )
    set_property(TARGET mc-afker PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
else()
    set_target_properties(mc-afker PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    # Add rpath for Linux to find shared libraries
    set_target_properties(mc-afker PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()

# Installation
install(TARGETS mc-afker
    RUNTIME DESTINATION bin
)

