cmake_minimum_required(VERSION 2.8)
if(COMMAND cmake_policy)
     cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

set(projName "maskToPolygon")
project(${projName})

set(CMAKE_C_FLAGS "-std=c99")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

option(BOUNDARY_EDIT "Edit polygon boundaries. Requires the Boost library" OFF)

####
# openCV
find_package( OpenCV REQUIRED )
include_directories(${OpenCV_INCLUDE_DIRS})
set(Libraries
    ${OpenCV_LIBS}
    ${Libraries}
)

####
# Find Boost, if BOUNDARY_EDIT is ON
if(BOUNDARY_EDIT) 
	find_package( Boost REQUIRED )
    include_directories(${Boost_INCLUDE_DIR} polygon_edit/)
	add_definitions(-DBOUNDARY_EDIT)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_INCLUDE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/include)

add_subdirectory(src)
