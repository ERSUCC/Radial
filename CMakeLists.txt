cmake_minimum_required(VERSION 4.4)

project(radial)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(radial src/build.cpp
                      src/exception.cpp
                      src/main.cpp
                      src/options.cpp
                      src/toml.cpp
                      src/utils.cpp)

if (NOT WIN32)
  target_compile_options(radial PRIVATE -O3)
endif ()

target_compile_definitions(radial PRIVATE RADIAL_VERSION="${RADIAL_VERSION}")

target_include_directories(radial PRIVATE include)
