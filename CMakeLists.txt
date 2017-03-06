cmake_minimum_required(VERSION 3.7)
project(nblock_ecap_adapter)

# Set the output folder where your program will be created
set(MODDIR "/usr/local/lib/")
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

############################################################
#                        ad-block                          #
############################################################

# Include some stuff
include_directories("/usr/include/nodejs/src/")
include_directories("/usr/include/nodejs/deps/v8/include/")
include_directories("src/node_modules/ad-block/node_modules/bloom-filter-cpp")
include_directories("src/node_modules/ad-block/node_modules/hashset-cpp")

file(GLOB SOURCES "src/node_modules/ad-block/*.cc")
add_library("ad-block" STATIC ${SOURCES})

############################################################
#                     ecap Adapter                         #
############################################################

# Include some stuff
include_directories("src/node_modules/ad-block/")
include_directories("src/node_modules/ad-block/node_modules/bloom-filter-cpp")
include_directories("src/node_modules/ad-block/node_modules/hashset-cpp")

file(GLOB SOURCES "src/*.cc")
add_library(${PROJECT_NAME} SHARED ${SOURCES})

find_package(PkgConfig)
PKG_CHECK_MODULES(LIBECAP libecap)
IF(NOT LIBECAP_FOUND)
  find_library(LIBECAP ecap)
  IF(NOT LIBECAP)
  message(FATAL_ERROR "
  The ECAP library is required to build nBlock.
  The latest version is available at http://www.e-cap.org/."
  )
  ENDIF()
  set(LIBECAP_LDFLAGS "-lecap")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DV003")
ELSE()
  IF(NOT LIBECAP_VERSION VERSION_LESS "1.0.0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DV100")
  ENDIF()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLIBECAP_VERSION=\\\"${LIBECAP_VERSION}\\\"")
ENDIF()

set ( PROJECT_LINK_LIBS ad-block.a )
link_directories( bin/ )

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} ${LIBECAP_LDFLAGS} ${PROJECT_LINK_LIBS})
include_directories(${LIBECAP_INCLUDE_DIRS})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${MODDIR})