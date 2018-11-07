project(crawler)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_STANDARD 11)

SET(FLAG_O2 "-O2")


# Point to our own cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)


FIND_PACKAGE(PkgConfig REQUIRED)


# Add global definitions
add_definitions("-Wall")

# MPI
FIND_PACKAGE(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
find_package(Boost REQUIRED mpi serialization)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(CrawlerSequencial crawler_url_da_listagem_por_categoria_sequencial.cpp)
target_link_libraries(CrawlerSequencial  "-lcurl")
SET_TARGET_PROPERTIES(CrawlerSequencial PROPERTIES COMPILE_FLAGS ${FLAG_O2} LINK_FLAGS ${FLAG_O2} )

add_executable(crawlerDistribuido Crawler_MPI.cpp)
target_link_libraries(crawlerDistribuido ${MPI_LIBRARIES} "-lcurl -lboost_mpi -lcurl -lboost_serialization -std=c++11")
SET_TARGET_PROPERTIES(crawlerDistribuido PROPERTIES COMPILE_FLAGS ${FLAG_O2} LINK_FLAGS ${FLAG_O2})



