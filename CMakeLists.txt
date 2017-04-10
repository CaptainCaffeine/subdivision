cmake_minimum_required(VERSION 3.1)
project(Subdivision)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(WARNING_FLAGS "-Wall -Wextra -Wshadow")
set(DEBUG_FLAGS "-fsanitize=undefined -fno-omit-frame-pointer")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${WARNING_FLAGS} ${DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${WARNING_FLAGS} ${DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")

find_package(glfw3 REQUIRED)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/glm")

add_subdirectory(src)

add_custom_target(copy_shader_files
                  COMMAND ${CMAKE_COMMAND} -E 
                  copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/shaders" "${CMAKE_CURRENT_BINARY_DIR}/shaders"
                  VERBATIM)

add_dependencies(subdivision copy_shader_files)
