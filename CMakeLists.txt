cmake_minimum_required(VERSION 3.30...4.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(PROJECT_VERSION "" CACHE STRING "Project version")
set(PROJECT_DESCRIPTION "" CACHE STRING "Project description")

project(CppTestLab VERSION ${PROJECT_VERSION}
                   DESCRIPTION ${PROJECT_DESCRIPTION}
                   LANGUAGES CXX)

add_executable(cpplab src/main.cpp)

target_compile_features(cpplab PUBLIC cxx_std_26)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES 
      ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

set_target_properties(cpplab PROPERTIES CXX_EXTENSIONS OFF)

install(TARGETS cpplab
        RUNTIME DESTINATION bin
)
