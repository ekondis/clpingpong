cmake_minimum_required(VERSION 3.1)
project(clpingpong)

# set(VERSION_MAJOR 1)
# set(VERSION_MINOR 1)
# set(VERSION_PATCH 0)

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

find_package(OpenCL)
if(NOT OpenCL_FOUND)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
  include(BuildIcd)
  find_package(OpenCL REQUIRED)
  # if(UNIX)
    # set(OpenCL_LIBRARIES ${OpenCL_LIBRARIES} pthread dl)
  # elseif(WIN32)
    # set(OpenCL_LIBRARIES ${OpenCL_LIBRARIES} cfgmgr32.lib)
  # endif()
endif()

FIND_PATH(HPP_FOUND CL/cl.hpp PATHS ${OpenCL_INCLUDE_DIRS})
if(NOT HPP_FOUND)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
  include(BuildCLHpp)
  set(OpenCL_INCLUDE_DIRS ${OpenCL_INCLUDE_DIRS} "${CLHPP_ROOT}/include")
endif()

if(NOT OpenCL_FOUND)
  message(FATAL_ERROR "Could not find OpenCL include/libs. Set OCL_ROOT to your OpenCL SDK")
else()
  message(STATUS "Selected OpenCL includes from ${OpenCL_INCLUDE_DIRS}")
  message(STATUS "Selected OpenCL lib ${OpenCL_LIBRARIES}")
endif()

set(CMAKE_CXX_STANDARD "11")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add_definitions("-DVERSION_MAJOR=${VERSION_MAJOR}")
# add_definitions("-DVERSION_MINOR=${VERSION_MINOR}")
# add_definitions("-DVERSION_PATCH=${VERSION_PATCH}")

function(make_includable input_file output_file)
    file(READ ${input_file} content)
    set(delim "for_c++_include")
    set(content "R\"${delim}(\n${content})${delim}\"")
    file(WRITE ${output_file} "${content}")
endfunction(make_includable)

# Use like
make_includable(src/kernels.cl autogen/kernels.h)

set(SOURCE_FILES
    src/main.cpp
    src/clpingpong.cpp
)

add_executable(clpingpong ${SOURCE_FILES})
target_link_libraries(clpingpong ${OpenCL_LIBRARIES})
target_include_directories(clpingpong PRIVATE ${OpenCL_INCLUDE_DIRS} "include" "src/kernels" "${CMAKE_CURRENT_SOURCE_DIR}/autogen")
target_compile_definitions(clpingpong PRIVATE CL_TARGET_OPENCL_VERSION=120)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
   CMAKE_CXX_COMPILER_ID STREQUAL "GNU"   OR
   CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  target_compile_options(clpingpong PRIVATE -fPIC -Wall -Wextra -Wconversion -Wno-deprecated-declarations -Wno-unused-parameter -Wno-ignored-attributes)
elseif(MSVC)
  target_compile_options(clpingpong PRIVATE /wd4996)
endif()

install(TARGETS clpingpong RUNTIME DESTINATION bin)
# install(FILES LICENSE DESTINATION share/clpeak)
