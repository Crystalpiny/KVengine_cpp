cmake_minimum_required(VERSION 3.26)
project(KVengine VERSION 1.0.0 DESCRIPTION "A Key-Value Engine using Skip List" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(HEADERS
        skiplist.h
        ThreadPool.h
        benchmark.h
)

set(SOURCES
        main.cpp
        benchmark.cpp
)

add_executable(KVengine ${SOURCES} ${HEADERS})