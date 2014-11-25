# Copyright (C) 2012, 2013, 2014
# Computer Graphics Group, University of Siegen.
# http://www.cg.informatik.uni-siegen.de/
# All rights reserved.
# Written by Martin Lambers <martin.lambers@uni-siegen.de>

cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0017 NEW)

# Configuration
project("PMDSim" C CXX)
set(PROJECT_VERSION "1.3")
add_definitions(-DPROJECT_VERSION=${PROJECT_VERSION})
if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
  add_definitions(-D_GNU_SOURCE)
endif()
if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-DNOMINMAX)
  add_definitions(-D_USE_MATH_DEFINES)
  # Add linker flag /OPT:NOREF, necessary to handle DLLs built by
  # binutils dlltool; see http://sourceware.org/bugzilla/show_bug.cgi?id=12633
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /OPT:NOREF")
endif()
if(MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--large-address-aware -mthreads -mwindows")
endif()

# Build options
option(STATIC_BUILD "Build statically linked binary (useful for MXE)" OFF)

# Find OpenGL and GLEW.
find_package(OpenGL REQUIRED)
find_package(GLEWMX REQUIRED)
add_definitions(-DGLEW_MX)
if(STATIC_BUILD)
  add_definitions(-DGLEW_STATIC)
endif()

# Find OpenSceneGraph.
set(OpenSceneGraph_MARK_AS_ADVANCED ON)
find_package(OpenSceneGraph REQUIRED COMPONENTS osgViewer osgDB osgGA osgUtil osgQt)
set(OPENSCENEGRAPH_PLUGIN_LIBRARIES "")
if(STATIC_BUILD)
  add_definitions(-DOSG_LIBRARY_STATIC)
  string(REGEX MATCH ".*/" OPENSCENEGRAPH_PLUGIN_DIR ${OSG_LIBRARY})
  set(OPENSCENEGRAPH_PLUGIN_DIR ${OPENSCENEGRAPH_PLUGIN_DIR}osgPlugins-${OPENSCENEGRAPH_VERSION})
  set(OPENSCENEGRAPH_PLUGIN_LIBRARIES -L${OPENSCENEGRAPH_PLUGIN_DIR} -losgdb_obj -losgdb_ply -losgdb_osg)
endif()

# Find Qt.
find_package(Qt4 REQUIRED QtCore QtGui QtOpenGL)
include(${QT_USE_FILE})

# Find GTA (optional).
find_package(GTA)
if(GTA_FOUND)
  add_definitions(-DHAVE_GTA)
endif()

# Main target
include(StringifyShaders)
stringify_shaders(
  src/render-simple.vs.glsl src/render-simple.fs.glsl
  src/reduction.fs.glsl
  src/simphaseadd.fs.glsl src/simresult.fs.glsl
  src/view2d.fs.glsl)
qt4_wrap_cpp(pmdsim_HEADERS_MOC
  src/glwidget.h
  src/simwidget.h
  src/osgwidget.h
  src/view2dwidget.h
  src/animwidget.h
  src/mainwindow.h)
qt4_add_resources(pmdsim_RESOURCES_RCC src/qt.qrc)
add_executable(pmdsim
  ${pmdsim_HEADERS_MOC} ${pmdsim_RESOURCES_RCC}
  src/main.cpp
  src/simulator.h src/simulator.cpp
  src/target.h src/target.cpp
  src/animation.h src/animation.cpp
  src/trianglepatch.h src/trianglepatch.cpp
  src/glhelper.inl src/simviewhelper.inl
  src/glwidget.h src/glwidget.cpp
  src/simwidget.h src/simwidget.cpp
  src/render-simple.vs.glsl.h src/render-simple.fs.glsl.h
  src/reduction.fs.glsl.h
  src/simphaseadd.fs.glsl.h src/simresult.fs.glsl.h
  src/osgwidget.h src/osgwidget.cpp
  src/view2dwidget.h src/view2dwidget.cpp
  src/view2d.fs.glsl.h
  src/animwidget.h src/animwidget.cpp
  src/mainwindow.h src/mainwindow.cpp)
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/src
  ${GTA_INCLUDE_DIRS} ${QT_INCLUDE_DIRS} ${OPENSCENEGRAPH_INCLUDE_DIRS} ${GLEWMX_INCLUDE_DIRS})
target_link_libraries(pmdsim
  ${GTA_LIBRARIES} ${QT_LIBRARIES}
  ${OPENSCENEGRAPH_PLUGIN_LIBRARIES} ${OPENSCENEGRAPH_LIBRARIES}
  ${GLEWMX_LIBRARIES} ${OPENGL_LIBRARIES})
install(TARGETS pmdsim RUNTIME DESTINATION bin)
install(FILES doc/animation-example.txt DESTINATION doc)

# Documentation (if doxygen is available)
find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file("${CMAKE_SOURCE_DIR}/doc/doxyfile.in" "${CMAKE_BINARY_DIR}/doc/doxyfile" @ONLY)
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/doc/html")
  add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/doc/html/index.html"
    COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_BINARY_DIR}/doc/doxyfile"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/doc"
    DEPENDS "${CMAKE_SOURCE_DIR}/doc/doxyfile.in"
            "${CMAKE_SOURCE_DIR}/doc/animation-example.txt"
            "${CMAKE_SOURCE_DIR}/src/README"
            "${CMAKE_SOURCE_DIR}/src/simulator.h"
            "${CMAKE_SOURCE_DIR}/src/target.h"
            "${CMAKE_SOURCE_DIR}/src/animation.h"
            "${CMAKE_SOURCE_DIR}/src/trianglepatch.h"
    COMMENT "Generating API documentation with Doxygen" VERBATIM
  )
  add_custom_target(doc ALL DEPENDS "${CMAKE_BINARY_DIR}/doc/html/index.html")
  install(DIRECTORY "${CMAKE_BINARY_DIR}/doc/html" DESTINATION doc)
endif()

# Extra target: 'make dist' for making distribution tarballs
set(ARCHIVE_NAME pmdsim-${PROJECT_VERSION})
add_custom_target(dist
  COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
    | xz > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.xz
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Extra target: 'make doc-dist' for making documentation tarballs
set(ARCHIVE_DOC_NAME pmdsim-${PROJECT_VERSION}-doc)
add_custom_target(doc-dist
  COMMAND ln -s doc/html ${ARCHIVE_DOC_NAME}
    && tar -hcvf - ${ARCHIVE_DOC_NAME} | xz > ${ARCHIVE_DOC_NAME}.tar.xz
    && rm ${ARCHIVE_DOC_NAME}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
