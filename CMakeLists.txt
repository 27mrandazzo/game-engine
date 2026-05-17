cmake_minimum_required(VERSION 3.30...4.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(PROJECT_VERSION "" CACHE STRING "Project version")
set(PROJECT_DESCRIPTION "" CACHE STRING "Project description")

set(CMAKE_COLOR_DIAGNOSTICS ON)

project(CppTestLab VERSION ${PROJECT_VERSION}
                   DESCRIPTION ${PROJECT_DESCRIPTION}
                   LANGUAGES CXX)

add_executable(cpplab)

target_compile_features(cpplab PUBLIC cxx_std_26)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES 
      ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

target_sources(cpplab
  PRIVATE
    src/main.cpp
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      src/engine.cppm
      src/util.cppm 
      src/dag.cppm
)

set_target_properties(cpplab PROPERTIES CXX_EXTENSIONS OFF)

find_package(SDL3 REQUIRED CONFIG)
target_link_libraries(cpplab PRIVATE SDL3::SDL3)

install(TARGETS cpplab
        RUNTIME DESTINATION bin
)
