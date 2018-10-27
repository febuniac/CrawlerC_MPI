project(crawler)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_STANDARD 11)

SET(FLAG_SEQUENCIAL "-O2")
SET(FLAG_PARALELO "-O2")

# Point to our own cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)


# Add global definitions
add_definitions("-Wall")

add_executable(CrawlerSequencial crawler_url_da_listagem_por_categoria_sequencial.cpp)
target_link_libraries(CrawlerSequencial  "-lcurl")
SET_TARGET_PROPERTIES(CrawlerSequencial PROPERTIES COMPILE_FLAGS ${FLAG_SEQUENCIAL} LINK_FLAGS ${FLAG_SEQUENCIAL} )

add_executable(CrawlerParalelo crawler_url_da_listagem_por_categoria_paralelo.cpp)

target_link_libraries(CrawlerParalelo  "-lcurl -lpthread")
SET_TARGET_PROPERTIES(CrawlerParalelo PROPERTIES COMPILE_FLAGS ${FLAG_PARALELO} LINK_FLAGS ${FLAG_PARALELO})

