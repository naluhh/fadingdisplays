cmake_minimum_required(VERSION 3.13)  # CMake version check
project(simple_example C)               # Create project "simple_example"

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package (Threads)
set(SOURCE_FILES main.c)

# search for pkg-config
include (FindPkgConfig)
if (NOT PKG_CONFIG_FOUND)
    message (FATAL_ERROR "pkg-config not found")
endif ()

# check for limping
pkg_check_modules (LIBPNG libpng16 REQUIRED)
if (NOT LIBPNG_FOUND)
    message(FATAL_ERROR "You don't seem to have libpng16 development libraries installed")
else ()
    include_directories (${LIBPNG_INCLUDE_DIRS})
    link_directories (${LIBPNG_LIBRARY_DIRS})
    link_libraries (${LIBPNG_LIBRARIES})
endif ()

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(image_driver main.c ${LIBPNG_LINK_FLAGS} )
target_link_libraries (image_driver ${CMAKE_THREAD_LIBS_INIT} m)
