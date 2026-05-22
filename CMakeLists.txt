cmake_minimum_required(VERSION 3.30...4.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(PROJECT_VERSION "" CACHE STRING "Project version")
set(PROJECT_DESCRIPTION "" CACHE STRING "Project description")

set(CMAKE_COLOR_DIAGNOSTICS ON)

add_compile_options(-stdlib=libstdc++)
add_link_options(-stdlib=libstdc++)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

project(CppTestLab VERSION ${PROJECT_VERSION}
                   DESCRIPTION ${PROJECT_DESCRIPTION}
                   LANGUAGES CXX)

add_library(game_modules)
target_compile_features(game_modules PUBLIC cxx_std_26)
set_target_properties(game_modules PROPERTIES CXX_EXTENSIONS OFF)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES 
      ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

target_sources(game_modules
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      src/engine.cppm
      src/util.cppm 
      src/dag.cppm
)

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)
target_link_libraries(game_modules PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)

add_executable(cpplab_ecs src/main-ecs.cpp)
target_compile_features(cpplab_ecs PUBLIC cxx_std_26)
set_target_properties(cpplab_ecs PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(cpplab_ecs PRIVATE game_modules)

# Naive version
add_executable(cpplab_oop src/main-oop.cpp)
target_compile_features(cpplab_oop PUBLIC cxx_std_26)
set_target_properties(cpplab_oop PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(cpplab_oop PRIVATE game_modules)

install(TARGETS cpplab_oop cpplab_ecs
        RUNTIME DESTINATION bin
)
