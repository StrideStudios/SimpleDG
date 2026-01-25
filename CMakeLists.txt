# CMake Parent File Requirements
cmake_minimum_required(VERSION 3.31...4.2)

# Project Definition
project(
        SimpleDG
        DESCRIPTION "A simple header only library that enables you to create easy dependency graphs"
        VERSION 1.0.0
        LANGUAGES CXX C
)

option(SIMPLEDG_TEST "Enable testing SimpleDG features" ${PROJECT_IS_TOP_LEVEL})

add_library(SimpleDG INTERFACE
    # Base Dependency Graph
    include/sdg/DependencyGraph.h
)

# If not overridden, DG CSS Standard is the same as parent
if(NOT SimpleDG_CXX_STANDARD)
    set(SimpleDG_CXX_STANDARD ${CMAKE_CXX_STANDARD})
endif ()

# If CXX Standard is not defined, fallback to c++17
if(NOT SimpleDG_CXX_STANDARD)
    set(SimpleDG_CXX_STANDARD 17)
    message(WARNING "SimpleDG: Could not find parent CXX standard; Using default.")
endif()

message(STATUS "SimpleDG: Using CXX version ${SimpleDG_CXX_STANDARD}")

# Create Macro with CXX Version
target_compile_definitions(SimpleDG INTERFACE CXX_VERSION=${SimpleDG_CXX_STANDARD})

# Ensure target is compiled with CXX Version
target_compile_features(SimpleDG INTERFACE "cxx_std_${SimpleDG_CXX_STANDARD}")

# Include Files
target_include_directories(SimpleDG INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

if(SIMPLEDG_TEST)
    enable_testing()
    add_subdirectory(test)
endif()