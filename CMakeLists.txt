cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

project(
  value_or_error
  VERSION 0.1
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".so")

include(third_party/module.cmake)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(doc)
