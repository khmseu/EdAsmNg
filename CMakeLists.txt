cmake_minimum_required(VERSION 3.15)

project(EdAsmNg_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

include(CTest)

add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
