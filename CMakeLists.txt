cmake_minimum_required(VERSION 3.12)
project(realtime_pipeline VERSION 1.0)

# Set the root of the DeepStream SDK
set (DEEPSTREAM_SDK_ROOT /opt/nvidia/deepstream/deepstream)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# CUDA and GStreamer
find_package(CUDA 12.2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)

if(CUDA_FOUND)
  find_cuda_helper_libs(nvinfer)
  message("-- CUDA_nvinfer_LIBRARY: " ${CUDA_nvinfer_LIBRARY})
  set(NVINFER_LIBRARY ${CUDA_nvinfer_LIBRARY} CACHE FILEPATH "location of the nvinfer library")

  find_cuda_helper_libs(nvonnxparser)
  set(NVONNXPARSER_LIBRARY ${CUDA_nvonnxparser_LIBRARY} CACHE FILEPATH "location of the nvonnxparser library")
  message("-- CUDA_nvonnxparser_LIBRARY: " ${NVONNXPARSER_LIBRARY})
  
  unset(CUDA_nvinfer_LIBRARY CACHE)
endif()

if(NVINFER_LIBRARY)
  find_path(NVINFER_INCLUDE_DIR
    NvInfer.h
    PATHS ${CUDA_TOOLKIT_INCLUDE}
    DOC "location of NvInfer.h"
    NO_DEFAULT_PATH
  )

  if(NOT NVINFER_INCLUDE_DIR)
    find_path(NVINFER_INCLUDE_DIR
        NvInfer.h
        DOC "location of NvInfer.h"
    )
  endif()

  message("-- Found NVINFER: " ${NVINFER_LIBRARY})
  message("-- Found NVINFER include: " ${NVINFER_INCLUDE_DIR})
endif()

include_directories(${NVINFER_INCLUDE_DIR})

# Include directories
include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${DEEPSTREAM_SDK_ROOT}/sources/includes)
include_directories(${GST_INCLUDE_DIRS})
# Link directories
set(DEEPSTREAM_LIB_DIRS ${DEEPSTREAM_SDK_ROOT}/lib)

# Application executable
file(GLOB SRCS "src/*.cpp")
add_executable(${PROJECT_NAME} ${SRCS})
# Include directories
include_directories(include)  # Add the include folder of your app
target_link_libraries(${PROJECT_NAME} ${GST_LIBRARIES} ${CUDA_LIBRARIES} nvdsgst_meta nvds_meta)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -O2)
target_link_directories(${PROJECT_NAME} PRIVATE ${CUDA_LIBRARY_DIRS} ${GST_LIBRARY_DIRS} ${DEEPSTREAM_LIB_DIRS})

# Shared library
file(GLOB LIB_SRCS "plugins/*.cpp")
set(LIB_NAME "yolov10n_bbox_parser")
add_library(${LIB_NAME} SHARED ${LIB_SRCS})
target_compile_options(${LIB_NAME} PRIVATE -fPIC -Wall)
target_link_libraries(${LIB_NAME} ${GST_LIBRARIES} ${CUDA_LIBRARIES} nvdsgst_meta nvds_meta)
target_link_directories(${LIB_NAME} PRIVATE ${CUDA_LIBRARY_DIRS} ${GST_LIBRARY_DIRS} ${DEEPSTREAM_LIB_DIRS})

# Additional configurations for debug/release
# Set default build type if not provided with -DCMAKE_BUILD_TYPE
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_BUILD_TYPE "Release")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -g)
endif()

# Install setup
#install(TARGETS ${PROJECT_NAME} ${LIB_NAME} DESTINATION /opt/nvidia/deepstream/deepstream/lib)
