cmake_minimum_required(VERSION 3.7)
project(nblock_ecap_adapter)

add_definitions(-DPACKAGE_NAME="\\"nBlock\ ecap\ adapter\\"")
add_definitions(-DPACKAGE_VERSION="\\"0.01\\"")

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

set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -std=c++11 -stdlib=libc++ -v") 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC") 

add_library("ad-block" STATIC 
  src/node_modules/ad-block/addon.cc
  src/node_modules/ad-block/ad_block_client_wrap.cc
  src/node_modules/ad-block/ad_block_client.cc
  src/node_modules/ad-block/cosmetic_filter.cc
  src/node_modules/ad-block/filter.cc
  src/node_modules/ad-block/node_modules/bloom-filter-cpp/BloomFilter.cpp
  src/node_modules/ad-block/node_modules/bloom-filter-cpp/hashFn.cpp
  src/node_modules/ad-block/node_modules/hashset-cpp/HashSet.cpp
)

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
  The latest version is available at http://www.e-cap.org/.
  For debian 'sudo apt-get install libecap3'"
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

include_directories(${LIBECAP_INCLUDE_DIRS})

set ( LIBADBLOCK_LINK_LIB ad-block )
link_directories( build/ )
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

target_link_libraries(${PROJECT_NAME} ${LIBECAP_LDFLAGS} ${LIBADBLOCK_LINK_LIB})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${MODDIR})
