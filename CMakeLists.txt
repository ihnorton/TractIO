cmake_minimum_required(VERSION 3.1)

#==============================================================================
# CMake Modules
include(ExternalProject)

#==============================================================================
#TODO
set (CMAKE_CXX_STANDARD 11)

#if (APPLE)
#    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -fvisibility=hidden -fvisibility-inlines-hidden ${no_warn}")
#endif()
#set(CMAKE_LINKER_FLAGS "-std=c++11 -stdlib=libc++")

#==============================================================================
# 
# VTK_DIR "Path to VTKConfig.cmake"
# DCMTK_DIR "Path to DCMTKConfig.cmake" ""

#==============================================================================
set(TractIO_VTK_COMPONENTS vtkCommonCore vtkIOLegacy
                           vtkIOXML vtkCommonDataModel
                           vtkFiltersGeneral)


#==============================================================================
# VTK
find_package(VTK 6.1 COMPONENTS ${TractIO_VTK_COMPONENTS} REQUIRED NO_MODULE)
include(${VTK_USE_FILE})
if(NOT DEFINED VTK_RENDERING_BACKEND)
  set(VTK_RENDERING_BACKEND "OpenGL")
endif()

#==============================================================================
# DCMTK
#find_package(DCMTK REQUIRED)
#message(STATUS "DCMTK: ${DCMTK_USE_FILE}")
#include(${DCMTK_USE_FILE})

include("/Users/inorton/work/git/dcmtk_trc/bld/DCMTKConfig.cmake")


#==============================================================================
# JSON

#ExternalProject_Add(JSON
#  GIT_REPOSITORY https://github.com/nlohmann/json
#  GIT_TAG v1.0.0
#  DOWNLOAD_DIR json-git
#  BUILD_COMMAND ""
#  INSTALL_COMMAND ""
#  )

#==============================================================================
add_subdirectory("src/")
add_subdirectory("test/")
